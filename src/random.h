#ifndef _RAND_H_
#define _RAND_H_

#include "lib/framework/types.h"

#ifdef __cplusplus

// Pseudorandom number generator with 19937 bit state and period of 2**19937 - 1. Equidistributed in up to 623 dimensions.
// See http://en.wikipedia.org/wiki/Mersenne_twister
// If all clients use the same seed, they will generate the same pseudorandom number sequence.
class MersenneTwister
{
public:
	MersenneTwister(uint32_t seed = 42);
	uint32_t u32();  ///< Generates a random number in the interval [0...UINT32_MAX].

private:
	void generate();  ///< Generates more random numbers.

	int offset;  ///< Index in state.
	uint32_t state[624];  ///< 19937 bit state, rounded up to next 32 bits.
};

extern "C"
{
#endif //_cplusplus

/// Seeds the random number generator. TODO The seed should be sent over the network, so that all clients generate the same number sequence.
void gameSRand(uint32_t seed);

/// Generates a random number in the interval [0...UINT32_MAX].
/// Must not be called from graphics routines, only for making game decisions.
uint32_t gameRandU32(void);

/// Generates a random number in the interval [0...limit - 1]. Not equidistributed for large non-powers of 2.
/// Must not be called from graphics routines, only for making game decisions.
int32_t gameRand(uint32_t limit);

#ifdef __cplusplus
}
#endif //_cplusplus

#endif //_RAND_H_
