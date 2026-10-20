// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVECHILDBULLETSTORM_FXH
#define CARBON_EVECHILDBULLETSTORM_FXH


// Data passed from the engine to the vertex shader stage for EveChildBulletStorm draw calls.
struct EveChildBulletStormVertexInput
{
	// Vertex index of the corner of the quad, from 0 to 3
    float cornerID : TEXCOORD0;
	// Bullet position in object space
    float3 positionOS : TEXCOORD1;
    // Bullet direction in object space
    float3 directionOS : TEXCOORD2;
    // XYZ - three independent random values per bullet (from 0 to 1), W - part ID
    float4 data : TEXCOORD3;
};


// Data passed from the engine to the vertex shader stage for EveChildBulletStorm draw calls.
struct EveChildBulletStormDataVS
{
	// World transform matrix
    float4x4 WorldMatrix;
	// Target information: X - number of targets (for TargetPositionAndSize), Y - effect range, Z - clip sphere radius, W - bullet speed
    float4 TargetPositionInfo;
    // Up to 10 target positions and sizes (XYZ = position, W = size)
    float4 TargetPositionAndSize[10];
};



#endif