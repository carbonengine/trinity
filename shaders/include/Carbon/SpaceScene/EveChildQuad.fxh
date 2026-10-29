// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVECHILDQUAD_FXH
#define CARBON_EVECHILDQUAD_FXH


struct EveChildQuadVertexInput
{
    // Quad vertex index (0-3)
    float index : TEXCOORD5;

    // Transform matrix (3x4) from owner local space to world space
    float4 parentTransform0 : POSITION0;
    float4 parentTransform1 : POSITION1;
    float4 parentTransform2 : POSITION2;

    // Transform matrix (3x4) from quad local space to owner local space
    float4 localTransform0 : POSITION3;
    float4 localTransform1 : POSITION4;
    float4 localTransform2 : POSITION5;

    // Quad color
    float4 color : TEXCOORD0;
    // X - quad brightness, Y - unused
    float2 data : TEXCOORD1;
};


#endif