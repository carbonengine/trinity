// Copyright © 2026 CCP ehf.

#ifndef CARBON_INTERIOR_PEROBJECTDATA_FXH
#define CARBON_INTERIOR_PEROBJECTDATA_FXH

#include "../System.fxh"

// Constant buffer format for per-object data for interior scene objects. This data is provided by the engine to the vertex shader stage.
struct InteriorObjectDataVS
{
    // World transform of this object
    float4x4 WorldMat;
    float4 boundingBoxData;
};

struct InteriorSkinnedObjectDataVS
{
    // Set this up as a column-major 4x3 matrix, which is like a
    // row-major 3x4. This allows us to use the same mul syntax
    // as we normally do, but still allow packing of a 3x4 into
    // 3 registers.
    column_major float4x3 JointMat[69];
    float4x4 WorldMat;
    float3 WorldPos;
};


// interior
struct InteriorObjectLightData
{
    float4 position; // position in [x,y,z], radius in [w]
    float4 color; // color in [r,g,b], falloff in [a]
    float4 exData; // additional data: [x] = shadow0 influence, [y] = shadow1 influence, [z] = cos of spotlight outer cone alpha, [w] = cos of spotlight inner cone alpha
							 // or box light's world to box transform row 1 for box lights
    float4 spotDirection; // spotlight's direction: [x,y,z} = normalized direction
							 // or box light's world to box transform row 2 for box lights
    float4 boxTransformRow3; // box light's world to box transform row 3
    float4 boxTransformRow4; // box light's world to box transform row 4
};

#define NUM_INTERIORLIGHTS 10

// Constant buffer format for per-object data for interior scene objects. This data is provided by the engine to the pixel shader stage.
struct InteriorObjectDataPS
{
    int4 pointLightsCount; // yzw - unused

	// data on lights
    InteriorObjectLightData lights[NUM_INTERIORLIGHTS];
	// data on the shadow casting point lights: [x,y,z] = position, [w] = radius
    float4 shadowCaster0;
    float4 shadowCaster1;
    // spotlight view projection matrices
    float4x4 spotLights[4];
};



#endif