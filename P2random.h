// Project identifier: 19034C8F3B1196BF8E0C6E1C0F973D2FD550B88F

#ifndef P2RANDOM_H
#define P2RANDOM_H

#include <sstream>
#include <vector>
#include <string>

class P2random {
public:
    // Call this function when PR mode is designated, using the parameters
    // from the input file
    static void PR_init(std::stringstream& ss,
                        uint32_t size,
                        uint32_t seed,
                        uint32_t max_rubble,
                        uint32_t tnt);

    // No need to read further than this, unless you want to learn about
    // Mersenne Twister implementation

private:
    //
    // mt.h: Mersenne Twister header file
    //
    // Jason R. Blevins <jrblevin@sdf.lonestar.org>
    // Durham, March  7, 2007
    //

    //
    // Mersenne Twister.
    //
    // M. Matsumoto and T. Nishimura, "Mersenne Twister: A
    // 623-dimensionally equidistributed uniform pseudorandom number
    // generator", ACM Trans. on Modeling and Computer Simulation Vol. 8,
    // No. 1, January pp.3-30 (1998).
    //
    // http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html.
    //
    class MersenneTwister
    {
    public:
        MersenneTwister(void);
        ~MersenneTwister(void);

        // The copy constructor and operator=() should never be used.
        MersenneTwister(const MersenneTwister&) = delete;
        MersenneTwister &operator=(const MersenneTwister&) = delete;

        void init_genrand(uint32_t s);

        uint32_t genrand_unsigned_int(void);

    private:
        static const uint32_t N          = 624;
        static const uint32_t M          = 397;
        // constant vector a
        static const uint32_t MATRIX_A   = 0x9908b0dfU;
        // most significant w-r bits
        static const uint32_t UPPER_MASK = 0x80000000U;
        // least significant r bits
        static const uint32_t LOWER_MASK = 0x7fffffffU;

        uint32_t *mt_;  // the state vector
        uint32_t mti_;  // mti == N+1 means mt not initialized
    };

    static MersenneTwister mt;
};

#endif  // P2RANDOM_H
