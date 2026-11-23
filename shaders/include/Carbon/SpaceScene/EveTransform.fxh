// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVETRANSFORM_FXH
#define CARBON_EVETRANSFORM_FXH


// Data passed from the engine to the vertex shader stage for EveTransform draw calls.
struct EveTransformDataVS
{
	// World transform of this object
    float4x4 WorldMat;
    // World transform of this object in the previous frame
    float4x4 WorldMatLast;
	// Inverse world transform of this object
    float4x4 InvWorldMat;
};


#endif