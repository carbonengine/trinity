// Copyright © 2026 CCP ehf.

#ifndef CARBON_PEROBJECTDATA_FXH
#define CARBON_PEROBJECTDATA_FXH

#include "../BoneTransforms.fxh"

// Constant buffer format for per-object data for space scene objects. This data is provided by the engine to the vertex shader stage.
struct EveSpaceObjectDataVS
{
	// world transform of this object
    float4x4 WorldMat;
    // world transform of this object in the previous frame
    float4x4 WorldMatLast;
	// inverse world transform of this object
    float4x4 InvWorldMat;
	// additional ship-data ( components: .x = boosterGain, .y = activation, .z = unused, .w = boundingsphere radius )
    float4 Shipdata;
	// clip data for disolve fx ( components: .xyz = center, .w = signed squared size )
    float4 Clipdata1;
	// bounding ellipsoid data
    float4 EllipsoidRadii;
    float4 EllipsoidCenter;
	// custom mask data for pattern projection
    float4x4 CustomMaskMatrix[2];
	// additional custommap-data ( components: .x = layerCount, components: .y = isMirrored,  )
    float4 CustomMaskData[2];

    uint4 BoneOffsets; // .x = current frame offset, .y = previous frame offset
    uint4 MorphTarget; // .x = VertexDataOffset, .y = AnimationDataOffset, .z = MorphTargetsCount, .w = BakedVertexDataOffset

    float4 CustomData;
};

// Constant buffer format for per-object data for space scene objects. This data is provided by the engine to the pixel shader stage.
struct EveSpaceObjectDataPS
{
	// world transform of this object
    float4x4 WorldMat;
    // world transform of this object in the previous frame
    float4x4 WorldMatLast;
	// inverse world transform of this object
    float4x4 InvWorldMat;

	// additional ship-data ( components: .x = boosterGain, .y = activation, .z = dirt level, .w = boundingsphere radius )
    float4 Shipdata;
	// clip data for disolve fx ( components: .xyz = center, .w = signed squared size )
    float4 Clipdata1;
	// misc data ( .x = signed squared clip sphere 2 radius, .y = impact data offset, .z = clip sphere 2 factor, .w = clip sphere factor)
    float4 Miscdata;
	// SH lighting coefficients
    float4 ShLighting[7];
	// custom mask material ids ( components: .x = material1, .y = material2, .z = material3, .w = material4 )
    float4 CustomMaskMaterialIDs[2];
	// custom mask targets ( components: .x = isTargetMtl1, .y = isTargetMtl2, .z = isTargetMtl3, .w = isTargetMtl4 )
    float4 CustomMaskTargets[2];
    float4 CustomMaskClamps;
    float4 ScreenSize;
    float4 CustomData;
};

// Unified buffer per-object data for space scene objects. Used by shared instanced rendering and read from InstancedPerObjectBuffer.
// It is also possible to construct this data from the VS and PS per-object data structures, but some fields will be zeroed out.
// See CreateEveSpaceObjectData() functions below.
struct EveSpaceObjectData
{
	// world transform of this object
    float4x4 WorldMat;
    // world transform of this object in the previous frame
    float4x4 WorldMatLast;
    // inverse world transform of this object
    float4x4 InvWorldMat;
	// additional ship-data ( components: .x = boosterGain, .y = activation, .z = unused, .w = boundingsphere radius )
    float4 Shipdata;
	// clip data for disolve fx ( components: .xyz = center, .w = signed squared size )
    float4 Clipdata1;
	// misc data ( .x = signed squared clip sphere 2 radius, .y = impact data offset, .z = clip sphere 2 factor, .w = clip sphere factor)
    float4 Miscdata;

	// bounding ellipsoid data
    float4 EllipsoidRadii;
    float4 EllipsoidCenter;
	
	// custom mask data
    float4x4 CustomMaskMatrix[2];
	// additional custommap-data ( components: .x = layerCount, components: .y = isMirrored,  )
    float4 CustomMaskData[2];
	// custom mask material ids ( components: .x = material1, .y = material2, .z = material3, .w = material4 )
    float4 CustomMaskMaterialIDs[2];
	// custom mask targets ( components: .x = isTargetMtl1, .y = isTargetMtl2, .z = isTargetMtl3, .w = isTargetMtl4 )
    float4 CustomMaskTargets[2];
    float4 CustomMaskClamps;

 	// .x = current frame offset, .y = previous frame offset
    uint4 BoneOffsets;
	// .x - tactical camera object type, .y - tactical view transparency type
    float4 CustomData;
	// SH lighting coefficients
    float4 ShLighting[7];
};


EveSpaceObjectData CreateEveSpaceObjectData( EveSpaceObjectDataVS vsData )
{
    EveSpaceObjectData perObjectData;
    perObjectData.WorldMat = vsData.WorldMat;
    perObjectData.WorldMatLast = vsData.WorldMatLast;
    perObjectData.InvWorldMat = vsData.InvWorldMat;
    perObjectData.Shipdata = vsData.Shipdata;
    perObjectData.Clipdata1 = vsData.Clipdata1;
    perObjectData.Miscdata = 0;
    perObjectData.EllipsoidRadii = vsData.EllipsoidRadii;
    perObjectData.EllipsoidCenter = vsData.EllipsoidCenter;
    perObjectData.CustomMaskMatrix[0] = vsData.CustomMaskMatrix[0];
    perObjectData.CustomMaskMatrix[1] = vsData.CustomMaskMatrix[1];
    perObjectData.CustomMaskData[0] = vsData.CustomMaskData[0];
    perObjectData.CustomMaskData[1] = vsData.CustomMaskData[1];
    perObjectData.CustomMaskMaterialIDs[0] = 0;
    perObjectData.CustomMaskMaterialIDs[1] = 0;
    perObjectData.CustomMaskTargets[0] = 0;
    perObjectData.CustomMaskTargets[1] = 0;
    perObjectData.CustomMaskClamps = 0;
    perObjectData.BoneOffsets = vsData.BoneOffsets;
    perObjectData.CustomData = vsData.CustomData;
    perObjectData.ShLighting[0] = 0;
    perObjectData.ShLighting[1] = 0;
    perObjectData.ShLighting[2] = 0;
    perObjectData.ShLighting[3] = 0;
    perObjectData.ShLighting[4] = 0;
    perObjectData.ShLighting[5] = 0;
    perObjectData.ShLighting[6] = 0;
    return perObjectData;
}

EveSpaceObjectData CreateEveSpaceObjectData( EveSpaceObjectDataPS psData )
{
    EveSpaceObjectData perObjectData;
    perObjectData.WorldMat = psData.WorldMat;
    perObjectData.WorldMatLast = psData.WorldMatLast;
    perObjectData.InvWorldMat = psData.InvWorldMat;
    perObjectData.Shipdata = psData.Shipdata;
    perObjectData.Clipdata1 = psData.Clipdata1;
    perObjectData.Miscdata = psData.Miscdata;
    perObjectData.EllipsoidRadii = 0;
    perObjectData.EllipsoidCenter = 0;
    perObjectData.CustomMaskMatrix[0] = float4x4( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    perObjectData.CustomMaskMatrix[1] = float4x4( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    perObjectData.CustomMaskData[0] = 0;
    perObjectData.CustomMaskData[1] = 0;
    perObjectData.CustomMaskMaterialIDs[0] = psData.CustomMaskMaterialIDs[0];
    perObjectData.CustomMaskMaterialIDs[1] = psData.CustomMaskMaterialIDs[1];
    perObjectData.CustomMaskTargets[0] = psData.CustomMaskTargets[0];
    perObjectData.CustomMaskTargets[1] = psData.CustomMaskTargets[1];
    perObjectData.CustomMaskClamps = psData.CustomMaskClamps;
    perObjectData.BoneOffsets = 0;
    perObjectData.CustomData = psData.CustomData;
	
    perObjectData.ShLighting[0] = psData.ShLighting[0];
    perObjectData.ShLighting[1] = psData.ShLighting[1];
    perObjectData.ShLighting[2] = psData.ShLighting[2];
    perObjectData.ShLighting[3] = psData.ShLighting[3];
    perObjectData.ShLighting[4] = psData.ShLighting[4];
    perObjectData.ShLighting[5] = psData.ShLighting[5];
    perObjectData.ShLighting[6] = psData.ShLighting[6];
	// intentionally skipping ScreenSize as I don't think it's used
    return perObjectData;
}

// This buffer contains per-object data for shared instanced rendering.
StructuredBuffer<EveSpaceObjectData> InstancedPerObjectBuffer <bool AutoRegister = true; >;


float4x4 GetBoneMatrix( EveSpaceObjectData perObjectData, int boneIndex )
{
    return GetBoneTransform( perObjectData.BoneOffsets.x + boneIndex );
}

float4x4 GetLastBoneMatrix( EveSpaceObjectData perObjectData, int boneIndex )
{
    return GetBoneTransform( perObjectData.BoneOffsets.y + boneIndex );
}

float4x4 GetBoneMatrix( EveSpaceObjectDataVS perObjectData, int boneIndex )
{
    return GetBoneTransform( perObjectData.BoneOffsets.x + boneIndex );
}

float4x4 GetLastBoneMatrix( EveSpaceObjectDataVS perObjectData, int boneIndex )
{
    return GetBoneTransform( perObjectData.BoneOffsets.y + boneIndex );
}


#endif