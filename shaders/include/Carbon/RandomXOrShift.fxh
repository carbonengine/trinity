// Copyright © 2026 CCP ehf.

#ifndef CARBON_RANDOM_FXH
#define CARBON_RANDOM_FXH


// Based on https://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/

// Initializes a random number generator state based on a seed
uint RandInit( uint seed )
{
    seed = ( seed ^ 61 ) ^ ( seed >> 16 );
    seed *= 9;
    seed = seed ^ ( seed >> 4 );
    seed *= 0x27d4eb2d;
    seed = seed ^ ( seed >> 15 );
    return seed;
}

// Returns a random uint in the range [0, 2^32 - 1] and updates the rngState
uint Rand( inout uint rngState )
{
    // Xorshift algorithm from George Marsaglia's paper
    rngState ^= ( rngState << 13 );
    rngState ^= ( rngState >> 17 );
    rngState ^= ( rngState << 5 );
    return rngState;
}

// Returns a random float in the range [0, 1) and updates the rngState
float RandFloat( inout uint rngState )
{
    return float( Rand( rngState ) ) * ( 1.0 / 4294967296.0 );
}



#endif