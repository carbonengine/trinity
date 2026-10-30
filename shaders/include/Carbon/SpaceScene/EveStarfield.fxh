// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVESTARFIELD_FXH
#define CARBON_EVESTARFIELD_FXH

struct EveStarfieldVertexInput
{
    // Start sprite position in local space
    float3 position : POSITION;
    // Random number from 0 to 1 for each star
    float coloridx : TEXCOORD0;
    // Maximum intensity of the star sprite
    float flashIntensity : TEXCOORD1;
    // Random phase for the star sprite's flashing effect (0 to 1)
    float phase : TEXCOORD2;
    // Random rate for the star sprite's flashing effect
    float rate : TEXCOORD3;
    // X - vertex corner index (0-3), Y - texture atlas element index, Z - unused, W - unused
    uint4 index : TEXCOORD4;
};


#endif