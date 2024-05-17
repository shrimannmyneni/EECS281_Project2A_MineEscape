// Project identifier: 19034C8F3B1196BF8E0C6E1C0F973D2FD550B88F
#include <iostream>
#include <getopt.h>
#include <queue>
#include <cmath>
#include <string>
#include <iomanip>
#include <algorithm>
#include <vector>
#include "P2random.h"

struct Tile{
    uint32_t x,y;
    int rubble;
    bool isTNT; //true if a Tile is -1
    bool isDiscovered = false;

    Tile(uint32_t _x_, uint32_t _y_, int _rubble_, bool _isTNT_)
        : x(_x_), y (_y_), rubble(_rubble_), isTNT(_isTNT_), isDiscovered(false) {}

};

struct TileComparator {
    bool operator() (const Tile& a, const Tile& b) const {
        if (a.isTNT && !b.isTNT) return false; // a is easier because it's TNT
        if (!a.isTNT && b.isTNT) return true;  // b is easier because it's TNT

        // For non-TNT tiles or both TNT: prioritize by lowest rubble-value
        if (a.rubble != b.rubble) return a.rubble > b.rubble;

        // Tie-breaking by lowest column number (x coordinate)
        if (a.x != b.x) return a.x > b.x;

        // Final tie-break by lowest row number (y coordinate)
        return a.y > b.y;
    }
};

class MineEscape{
    private: 
    bool verboseMode = false;
    bool statsMode = false;
    bool medianMode = false;
    int statsValue = 0;

    uint32_t gridSize = 0;
    uint32_t x;
    uint32_t y;
    std::vector<std::vector<Tile>> map2D;

    std::priority_queue <Tile, std::vector<Tile>, TileComparator> primary_pq;
    std::priority_queue <Tile, std::vector<Tile>, TileComparator> tnt_pq;
    int total_rubble = 0;
    size_t non_tnt_num_tiles_cleared = 0;
    size_t tnt_num_tiles_cleared = 0;
    bool escaped = false;

    // output stuff
    std::priority_queue<int> median_max_heap;
    std::priority_queue<int, std::vector<int>, std::greater<int>> median_min_heap;

    std::vector<std::pair<int/*rubble*/, std::pair<uint32_t/*x*/,uint32_t/*y*/>>> stats_ov;
    // std::priority_queue <Tile, std::vector<Tile>, TileComparator> verbose_tnt_pq;

    public:
    MineEscape() : verboseMode(false), statsMode(false), medianMode(false), statsValue(0), 
    gridSize(0), x(0), y(0), total_rubble(0), non_tnt_num_tiles_cleared(0), 
    tnt_num_tiles_cleared(0), escaped(false) {}

    
    void /*MineEscape::*/get_options(int argc, char* argv[]){
        int optionIdx = 0, option = 0;
        opterr = false;

        struct option longOpts [] = {
            {"help", no_argument, nullptr, 'h'},
            {"verbose", no_argument, nullptr, 'v'},
            {"median", no_argument, nullptr, 'm'},
            {"stats", required_argument, nullptr, 's'},
            {nullptr, no_argument, nullptr, '\0'}
        };
        
    while ((option = getopt_long(argc, argv, "hvms:", longOpts, &optionIdx)) != -1){
            switch(option){
                case 'h':
                    std::cerr << "help\n";
                    exit(0);

                case 'v':
                    std::cerr << "Verbose Mode" << '\n';
                    verboseMode = true;
                    break;

                case 'm':
                    std::cerr << "Median Mode" << '\n';
                    medianMode = true;
                    break;

                case 's':
                    std::cerr << "Stats Mode" << '\n';
                    statsMode = true;
                    statsValue = std::stoi(optarg);
                    break;

                default:
                    std::cerr << "Unknown option" << '\n';
                    exit(1);
            }
        }
    }//end of get_options

    void /*MineEscape::*/read_header(){
        std::string mode;
        std::cin >> mode;
        if (mode != "R" && mode != "M"){
        std::cerr << "Invalid mode: must be 'R' or 'M'" << '\n';
        exit(1);
        }
        
        int potential_x;
        int potential_y;

        std::string junk;
        std::cin >> junk >> gridSize >> junk >> potential_y >> potential_x;

        if (potential_x >= static_cast<int>(gridSize) || potential_x < 0 || potential_y >= static_cast<int>(gridSize) || potential_y < 0 ){
        std::cerr << "invalid coordinates" << '\n';
        exit(1);
        }else{
            y = static_cast<uint32_t>(potential_y);
            x = static_cast<uint32_t>(potential_x);
        }

        std::stringstream ss;
    if (mode == "R") {
         uint32_t seed;
         uint32_t max_rubble;
         uint32_t tnt;
        std::cin >> junk >> seed >> junk >> max_rubble >> junk >> tnt;
        P2random::PR_init(ss, gridSize, seed, max_rubble, tnt);
    }  // if ..'R'

    

    std::istream &inputStream = (mode == "M") ? std::cin : ss;
    
    map2D.reserve(gridSize);  // Optionally reserve space for gridSize vectors for efficiency
    for (uint32_t i = 0; i < gridSize; ++i) {
        std::vector<Tile> row;  // Create a new row for each i
        row.reserve(gridSize);  // Optionally reserve space for gridSize Tiles for efficiency
        for (uint32_t j = 0; j < gridSize; ++j) {
            int rubble;
            inputStream >> rubble;
            bool is_tnt = (rubble == -1);
            row.emplace_back(j, i, rubble, is_tnt);  // Construct a Tile in place
        }
        map2D.push_back(std::move(row));  // Add the row to map2D
    }
} // end of read_header

void /*MineEscape::*/escape(){
    discover();

    Tile& startTile = map2D[y][x];
    if (startTile.isTNT) {
        output(startTile, false);
        determine_tnt_detonation_order(y, x);
        detonate_tnt();
        if (can_escape(y, x)) {
            escaped = true;
        }
    } else {
        if (startTile.rubble > 0) {
            total_rubble += startTile.rubble;
            output(startTile, false);
            non_tnt_num_tiles_cleared++;
            startTile.rubble = 0;
            if (can_escape(y, x)) {
            escaped = true;
        }
        }
        startTile.isDiscovered = true;
    }

    // Main loop to escape the mine
    while (!escaped) {
        discover();
        investigate();

    //     for (uint32_t i = 0; i < gridSize; ++i) {
    //     for (uint32_t j = 0; j < gridSize; ++j) {
    //             std::cout << map2D[i][j].rubble << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << '\n';
    // std::cout << y << " " << x << '\n';
    // std::cout << '\n';
    }

    std::cout << "Cleared " << non_tnt_num_tiles_cleared << " tiles containing " << total_rubble << " rubble and escaped." << '\n';
} // end of escape()

void /*MineEscape::*/discover(){
    if (y > 0){ // discover upper tile
        if (!map2D[y-1][x].isDiscovered){
            map2D[y-1][x].isDiscovered = true;
            primary_pq.push(map2D[y-1][x]);
        }
    }
    if (y < gridSize - 1){// discover lower tile
        if (!map2D[y+1][x].isDiscovered){
            map2D[y+1][x].isDiscovered = true;
            primary_pq.push(map2D[y+1][x]);
        }
    }
    if (x < gridSize - 1){// discover right tile
        if (!map2D[y][x+1].isDiscovered){
            map2D[y][x+1].isDiscovered = true;
            primary_pq.push(map2D[y][x+1]);
        }
    }
    if (x > 0){// discover left tile
        if (!map2D[y][x-1].isDiscovered){
            map2D[y][x-1].isDiscovered = true;
            primary_pq.push(map2D[y][x-1]);
        }
    }
}

void /*MineEscape:f:*/investigate(){
    Tile to_move_to = primary_pq.top();
    primary_pq.pop();

    x = to_move_to.x;
    y = to_move_to.y;

    if (to_move_to.rubble == 0 && can_escape(y,x)){
        escaped = true;
        return;
    }

    if (to_move_to.rubble == 0){
        return;
    }
    

    if (!map2D[y][x].isTNT && map2D[y][x].rubble > 0){
        total_rubble += map2D[y][x].rubble;
        output(map2D[y][x], false);
        map2D[y][x].rubble = 0;
        non_tnt_num_tiles_cleared++;
        if (can_escape(y,x)){escaped = true;}
    } else if (map2D[y][x].isTNT && map2D[y][x].rubble == -1){
        tnt_pq.push(to_move_to);

        // if (verboseMode){
        // verbose_tnt_pq.push(to_move_to);
        // }
        if (verboseMode){
        output(map2D[y][x], false);
        } 
        determine_tnt_detonation_order(y,x);
        detonate_tnt();

        //check if it is possible to escape or not
        Tile potential_escape_tile = primary_pq.top();
        uint32_t tempX = x;
        uint32_t tempY = y;
        x = potential_escape_tile.x;
        y = potential_escape_tile.y;
        // std::cout << "tried to escape at" << y << " " << x << std::endl;
        if (can_escape(y,x)){
            escaped = true;
        } else {
            x = tempX;
            y = tempY;
        }
    }
}



bool /*MineEscape::*/can_escape(uint32_t x, uint32_t y){
    // Check if the current position is at the edge of the grid
    return (x == 0 || x == gridSize - 1 || y == 0 || y == gridSize - 1);
}

void /*MineEscape::*/output(const Tile& t, bool clearedByTNT) {
    if (verboseMode){

        if (t.rubble != -1 && !clearedByTNT){
            std::cout << "Cleared: " << t.rubble << " at [" << t.y << ',' << t.x << ']' << '\n';
        } else if (clearedByTNT){
            std::cout << "Cleared by TNT: " << t.rubble << " at [" << t.y << "," << t.x << "]" << '\n';
        } else if (!clearedByTNT && t.rubble == -1){
            std::cout << "TNT explosion at [" << t.y << ',' << t.x << "]!" << '\n';
        }

    }

    if (medianMode){
    if (t.rubble > 0) { // Only include non-zero rubble value

        if (median_max_heap.empty() || t.rubble <= median_max_heap.top()) {
            median_max_heap.push(t.rubble);
        } else {
            median_min_heap.push(t.rubble);
        }


        if (median_max_heap.size() > median_min_heap.size() + 1) {
            median_min_heap.push(median_max_heap.top());
            median_max_heap.pop();
        } else if (median_min_heap.size() > median_max_heap.size()) {
            median_max_heap.push(median_min_heap.top());
            median_min_heap.pop();
        }

        std::cout << std::fixed << std::setprecision(2);
        if (median_max_heap.size() != median_min_heap.size()) {
            std::cout << "Median difficulty of clearing rubble is: " << static_cast<double>(median_max_heap.top()) << '\n';
        } else {
            double median = (static_cast<double>(median_max_heap.top()) + static_cast<double>(median_min_heap.top())) / 2.0;
            std::cout << "Median difficulty of clearing rubble is: " << median << '\n';
        }
    }
}


    if (statsMode && !t.isTNT){
        stats_ov.push_back(std::make_pair(t.rubble, std::make_pair(t.x, t.y)));
    }
}

void /*MineEscape::*/determine_tnt_detonation_order(uint32_t tnt_y, uint32_t tnt_x){
        //this will only be called recursively on other TNT tiles

        if (statsMode){
        stats_ov.push_back(std::make_pair(map2D[tnt_y][tnt_x].rubble, std::make_pair(map2D[tnt_y][tnt_x].x, map2D[tnt_y][tnt_x].y)));
        }
        
        map2D[tnt_y][tnt_x].rubble = 0;
        tnt_num_tiles_cleared++;
        

            if (tnt_y > 0){ // can we blow up upper tile
                if (map2D[tnt_y - 1][tnt_x].rubble != 0){
                    map2D[tnt_y - 1][tnt_x].isDiscovered = true;
                    tnt_pq.push(map2D[tnt_y - 1][tnt_x]);

                    // if (verboseMode){
                    // verbose_tnt_pq.push(map2D[tnt_y - 1][tnt_x]);
                    // }
                    
                    if (map2D[tnt_y - 1][tnt_x].rubble == -1){
                        if (verboseMode){
                             output(map2D[tnt_y - 1][tnt_x], false);
                        }  
                        determine_tnt_detonation_order(tnt_y - 1,tnt_x);
                    }
                } else /*if the rubble value is 0 let the TNT pq discover it for the normal pq*/ {
                    map2D[tnt_y - 1][tnt_x].isDiscovered = true;
                    primary_pq.push(map2D[tnt_y - 1][tnt_x]);
                }
            }
            if (tnt_y < gridSize - 1){// can we blow up lower tile
                if (map2D[tnt_y + 1][tnt_x].rubble != 0){
                    map2D[tnt_y + 1][tnt_x].isDiscovered = true;
                    tnt_pq.push(map2D[tnt_y + 1][tnt_x]);

                    // if (verboseMode){
                    // verbose_tnt_pq.push(map2D[tnt_y + 1][tnt_x]);
                    // }
                    
                    if (map2D[tnt_y + 1][tnt_x].rubble == -1){
                        if (verboseMode){
                             output(map2D[tnt_y + 1][tnt_x], false);
                        }
                        determine_tnt_detonation_order(tnt_y + 1,tnt_x);
                    }
                }  else /*if the rubble value is 0 let the TNT pq discover it for the normal pq*/ {
                    map2D[tnt_y + 1][tnt_x].isDiscovered = true;
                    primary_pq.push(map2D[tnt_y + 1][tnt_x]);
                }
            }
            if (tnt_x < gridSize - 1){// can we blow up right tile
                if (map2D[tnt_y][tnt_x + 1].rubble != 0){
                    map2D[tnt_y][tnt_x + 1].isDiscovered = true;
                    tnt_pq.push(map2D[tnt_y][tnt_x + 1]);

                    // if (verboseMode){
                    // verbose_tnt_pq.push(map2D[tnt_y][tnt_x + 1]);
                    // }
                    
                    if (map2D[tnt_y][tnt_x + 1].rubble == -1){
                        if (verboseMode){
                             output(map2D[tnt_y][tnt_x + 1], false);
                        }
                        determine_tnt_detonation_order(tnt_y,tnt_x + 1);
                    }
                } else /*if the rubble value is 0 let the TNT pq discover it for the normal pq*/ {
                    map2D[tnt_y][tnt_x + 1].isDiscovered = true;
                    primary_pq.push(map2D[tnt_y][tnt_x + 1]);
                }
            }
            if (tnt_x > 0){// can we blow up left tile
                if (map2D[tnt_y][tnt_x - 1].rubble != 0){
                    map2D[tnt_y][tnt_x - 1].isDiscovered = true;
                    tnt_pq.push(map2D[tnt_y][tnt_x - 1]);

                    // if (verboseMode){
                    // verbose_tnt_pq.push(map2D[tnt_y][tnt_x - 1]);
                    // }

                    if (map2D[tnt_y][tnt_x - 1].rubble == -1){
                        if (verboseMode){
                            output(map2D[tnt_y][tnt_x - 1], false);
                        }
                        determine_tnt_detonation_order(tnt_y,tnt_x - 1);
                    }
                } else /*if the rubble value is 0 let the TNT pq discover it for the normal pq*/ {
                    map2D[tnt_y][tnt_x - 1].isDiscovered = true;
                    primary_pq.push(map2D[tnt_y][tnt_x - 1]);
                }
            }
            
}

void detonate_tnt(){
    while (!tnt_pq.empty()){
        Tile t = tnt_pq.top();
        tnt_pq.pop();

        if (!tnt_pq.empty()){
        Tile second = tnt_pq.top();
            while (second.y == t.y && t.x == second.x && !tnt_pq.empty()){
            tnt_pq.pop();
            if (!tnt_pq.empty())
            second = tnt_pq.top();
            }
        }

        if (t.isTNT){
            // map2D[t.y][t.x].isTNT = false;
            primary_pq.push(map2D[t.y][t.x]);
        } else{
            output(t,true);
            total_rubble += map2D[t.y][t.x].rubble;
            non_tnt_num_tiles_cleared++;
            map2D[t.y][t.x].rubble = 0;
            primary_pq.push(map2D[t.y][t.x]);
        }

        if (can_escape(t.y,t.x)){
            escaped = true;
        }
        
        
    }
}

void stats_output(){
    if (!statsMode){return;}
    
    size_t total_cleared =  non_tnt_num_tiles_cleared + tnt_num_tiles_cleared;
    //If less than <N> tiles have been cleared, then simply print as many as you can
    size_t N = (total_cleared < static_cast<size_t>(statsValue)) ? total_cleared : static_cast<size_t>(statsValue);

    std::cout << "First tiles cleared:" << '\n';
    for (size_t i = 0; i < N; i++){
        if (stats_ov[i].first == -1){
            std::cout << "TNT at [" << stats_ov[i].second.second << "," << stats_ov[i].second.first << "]\n";
        } else {
            std::cout << stats_ov[i].first << " at [" << stats_ov[i].second.second << "," << stats_ov[i].second.first << "]\n";
        }
    }

    //reversse

    std::cout << "Last tiles cleared:" << '\n';
    for (size_t i = 0; i < N; i++){
        size_t index = stats_ov.size() - 1 - i;
        if (stats_ov[index].first == -1){
            std::cout << "TNT at [" << stats_ov[index].second.second << "," << stats_ov[index].second.first << "]\n";
        } else {
            std::cout << stats_ov[index].first << " at [" << stats_ov[index].second.second << "," << stats_ov[index].second.first << "]\n";
        }
    }

    //now we do easiest and hardest
    std::sort(stats_ov.begin(), stats_ov.end());

    std::cout << "Easiest tiles cleared:" << '\n';
    for (size_t i = 0; i < N; i++){
        if (stats_ov[i].first == -1){
            std::cout << "TNT at [" << stats_ov[i].second.second << "," << stats_ov[i].second.first << "]\n";
        } else {
            std::cout << stats_ov[i].first << " at [" << stats_ov[i].second.second << "," << stats_ov[i].second.first << "]\n";
        }
    }

    std::sort(stats_ov.rbegin(), stats_ov.rend());

    std::cout << "Hardest tiles cleared:" << '\n';
    for (size_t i = 0; i < N; i++){
        if (stats_ov[i].first == -1){
            std::cout << "TNT at [" << stats_ov[i].second.second << "," << stats_ov[i].second.first << "]\n";
        } else {
            std::cout << stats_ov[i].first << " at [" << stats_ov[i].second.second << "," << stats_ov[i].second.first << "]\n";
        }
    }
}
    
};

int main(int argc, char* argv[]){
    std::ios_base::sync_with_stdio(false);
    MineEscape m;
    m.get_options(argc, argv);
    m.read_header();
    m.escape();
    m.stats_output();
    return 0;
}