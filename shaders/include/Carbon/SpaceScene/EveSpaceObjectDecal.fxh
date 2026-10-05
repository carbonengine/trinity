// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVESPACEOBJECTDECAL_FXH
#define CARBON_EVESPACEOBJECTDECAL_FXH

#include "PerObjectData.fxh"
#include "../Math.fxh"


// Data passed from the engine to the vertex shader stage for EveSpaceObjectDecal draw calls.
struct EveSpaceObjectDecalDataVS
{
	// World transform of this decal's parent
    float4x4 WorldMat;
	// Inverse world transform of this decal's parent
    float4x4 InvWorldMat;
	// Decal projection to parent space (or to local bone) transform
    float4x4 DecalMatrix;
    // Inverse decal projection to parent space transform
    float4x4 InvDecalMatrix;
	// Transform from bone space to parent space
    float4x4 ParentBoneMatrix;
    // Inverse transform from parent space to bone space
    float4x4 InvParentBoneMatrix;
};


// Data passed from the engine to the pixel shader stage for EveSpaceObjectDecal draw calls.
struct EveSpaceObjectDecalDataPS
{
	// Parent ship's display data ( components: .x = killcounter, .yzw = unused )
    float4 DisplayData;
	// Additional ship-data ( components: .x = boosterGain, .y = activation, .z = dirt, .w = boundingsphere radius )
    float4 Shipdata;
	// Clip data for disolve fx ( components: .xyz = center, .w = signed size )
    float4 Clipdata1;
	// Misc data ( components: .x = signed square of clip radius 2,  .yzw = unused )
    float4 Miscdata;
	// SH lighting coefficients
    float4 ShLighting[7];
};



EveSpaceObjectData CreateEveSpaceObjectData( EveSpaceObjectDecalDataVS perObjectData )
{
    EveSpaceObjectData pod;
    pod.WorldMat = perObjectData.WorldMat;
    pod.WorldMatLast = (float4x4)0;
    pod.BoneOffsets = (uint4)0;

    pod.Shipdata = (float4)0;
    pod.Clipdata1 = (float4)0;
    pod.Miscdata = (float4)0;
    pod.EllipsoidRadii = (float4)0;
    pod.EllipsoidCenter = (float4)0;
    pod.CustomMaskMatrix[0] = IDENTITY_MATRIX;
    pod.CustomMaskMatrix[1] = IDENTITY_MATRIX;
    pod.CustomMaskData[0] = (float4)0;
    pod.CustomMaskData[1] = (float4)0;
    pod.CustomMaskMaterialIDs[0] = (float4)0;
    pod.CustomMaskMaterialIDs[1] = (float4)0;
    pod.CustomMaskTargets[0] = (float4)0;
    pod.CustomMaskTargets[1] = (float4)0;
    pod.CustomMaskClamps = (float4)0;
    pod.CustomData = (float4)0;
	// SH lighting coefficients
    for( int i = 0; i < 7; ++i )
    {
        pod.ShLighting[i] = 0;
    }
    return pod;
}


EveSpaceObjectData CreateEveSpaceObjectData( EveSpaceObjectDecalDataPS perObjectData )
{
    EveSpaceObjectData pod;
    pod.WorldMat = (float4x4)0;
    pod.WorldMatLast = (float4x4)0;
    pod.BoneOffsets = (uint4)0;

    pod.Shipdata = perObjectData.Shipdata;
    pod.Clipdata1 = perObjectData.Clipdata1;
    pod.Miscdata = perObjectData.Miscdata;
    pod.EllipsoidRadii = (float4)0;
    pod.EllipsoidCenter = (float4)0;
    pod.CustomMaskMatrix[0] = IDENTITY_MATRIX;
    pod.CustomMaskMatrix[1] = IDENTITY_MATRIX;
    pod.CustomMaskData[0] = (float4)0;
    pod.CustomMaskData[1] = (float4)0;
    pod.CustomMaskMaterialIDs[0] = (float4)0;
    pod.CustomMaskMaterialIDs[1] = (float4)0;
    pod.CustomMaskTargets[0] = (float4)0;
    pod.CustomMaskTargets[1] = (float4)0;
    pod.CustomMaskClamps = (float4)0;
    pod.CustomData = (float4)0;
	// SH lighting coefficients
    for( int i = 0; i < 7; ++i )
    {
        pod.ShLighting[i] = perObjectData.ShLighting[i];
    }
    return pod;
}



#endif