// Copyright © 2026 CCP ehf.

#ifndef CARBON_BEHAVIORGROUPBOOSTER_FXH
#define CARBON_BEHAVIORGROUPBOOSTER_FXH


// Data passed from the engine to the vertex shader stage for BehaviourGroupBooster draw calls.
struct BehaviourGroupBoosterVertexInput
{
    // Vertex position in local instance space
    float3 position : POSITION;
    // UV coordinates
    float2 texCoord : TEXCOORD0;
    // Owner agent position (xyz) and scale (w)
    float4 agentPos : TEXCOORD1;
    // Owner agent rotation quaternion
    float4 agentRot : TEXCOORD2;
    // Booster data: x=intensity, y= per agent random number (0 to 1), z=atlasIndex0, w=atlasindex1
    float4 boosterData : TEXCOORD3;
};


#endif