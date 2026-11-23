// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVETURRETSET_FXH
#define CARBON_EVETURRETSET_FXH

#include "PerObjectData.fxh"
#include "../Math.fxh"

#define EVE_MAX_TURRETS_PER_SET 24

struct EveTurrentSetPerObjectDataVS
{
	// X - "height" of the clipping plane for the turret set, Y - unused, Z - unused, W - unused
    float4 ClipData;
    // X - number of bones in the turret skeleton, Y - unused, Z - unused, W - unused
    float4 TurretData;
    // World transform of this object
    float4x4 WorldMat;
    // World transform of this object in the previous frame
    float4x4 WorldMatLast;
 	// X - current frame offset, Y - previous frame offset
    uint4 BoneOffsets;
    // Turret instances translation data (XYZ - position, W - 1.0)
    float4 TurretTrans[EVE_MAX_TURRETS_PER_SET];
    // Turret instances rotation quaternions
    float4 TurretRot[EVE_MAX_TURRETS_PER_SET];
};

struct EveTurrentSetPerObjectDataPS
{
	// Additional ship-data ( components: .x = boosterGain, .y = activation, .z = dirt level, .w = boundingsphere radius )
    float4 Shipdata;
	// Clip data for disolve fx ( components: .xyz = center, .w = signed size )
    float4 Clipdata1;
	// Misc data ( components: .x = signed square of clip radius 2,  .yzw = unused )
    float4 Miscdata;
	// SH lighting coefficients
    float4 ShLighting[7];
};


EveSpaceObjectData CreateEveSpaceObjectData( EveTurrentSetPerObjectDataVS PerObjectVS )
{
    EveSpaceObjectData pod;
    pod.WorldMat = PerObjectVS.WorldMat;
    pod.WorldMatLast = PerObjectVS.WorldMatLast;
    pod.BoneOffsets = PerObjectVS.BoneOffsets;

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
        pod.ShLighting[i] = (float4)0;
    }
    return pod;
}


EveSpaceObjectData CreateEveSpaceObjectData( EveTurrentSetPerObjectDataPS PerObjectPS )
{
    EveSpaceObjectData pod;
    pod.WorldMat = (float4x4)0;
    pod.WorldMatLast = (float4x4)0;
    pod.BoneOffsets = (uint4)0;

    pod.Shipdata = PerObjectPS.Shipdata;
    pod.Clipdata1 = PerObjectPS.Clipdata1;
    pod.Miscdata = PerObjectPS.Miscdata;
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
        pod.ShLighting[i] = PerObjectPS.ShLighting[i];
    }
    return pod;
}

#endif