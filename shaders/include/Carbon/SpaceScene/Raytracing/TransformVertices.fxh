// Copyright © 2026 CCP ehf.

#ifndef CARBON_RAYTRACING_TRANSFORMVERTICES_FXH
#define CARBON_RAYTRACING_TRANSFORMVERTICES_FXH

// Shared code for the compute shaders that write the vertex positions used to build ray tracing acceleration
// structures (Tr2RaytracingVertexTransforms). SkinVertices.fx is the default shader. A material replaces it for its mesh
// areas by adding a "TransformVertices" technique with a compute shader in pass 0 built on this file:
//
//  [numthreads( TRANSFORM_VERTICES_GROUP_SIZE, 1, 1 )]
//  void DisplaceVertices( uint3 groupId : SV_GroupID, uint3 threadId : SV_GroupThreadID )
//  {
//      TransformJob job;
//      uint vtx;
//      if( GetTransformVertex( groupId.x, threadId.x, job, vtx ) )
//      {
//          float3 pos = LoadPosition( job, vtx );
//          pos = ApplyMorphs( job, vtx, pos );
//          pos += LoadNormal( job, vtx ) * GetDisplacement( LoadTexCoord0( job, vtx ) );
//          pos = ApplySkinning( job, vtx, pos );
//          StoreTransformedPosition( job, vtx, pos );
//      }
//  }
//
//  technique TransformVertices
//  {
//      pass p0
//      {
//          ComputeShader = compile cs_5_0 DisplaceVertices();
//      }
//  }
//
// A TransformVertices technique runs again when the mesh's bones, morph targets or LOD change, or when the owner calls
// MarkBlasOutdated() on the area's Tr2RaytracingMeshArea (e.g. after changing a displacement parameter). Whether the
// result changes over time depends on how the material is used, not on the shader, so the mesh area decides: while
// Tr2MeshArea.updateCustomRtDisplacement is set, the technique runs every frame (e.g. while the displacement is
// animated). Only areas owned by a Tr2MeshArea have the flag; EveChildInstancedMeshes and EveTurretSet don't yet.
//
// One dispatch processes several jobs. A job transforms the vertices of one mesh LOD with one shader, and the mesh's
// areas that use that shader share it. Each job owns whole thread groups, so all threads of a thread group work on the
// same job. A job can continue in the next dispatch.

#include "../../System.fxh"
#include "../../BoneTransforms.fxh"
#include "../../VertexBufferUtil.fxh"
#include "../MeshMorphing.fxh"

// Number of threads in a thread group. Tr2RaytracingGeometry splits jobs into thread groups of this size.
#define TRANSFORM_VERTICES_GROUP_SIZE 64

// Marks an offset in TransformJob as unused.
#define TRANSFORM_VERTICES_INVALID_OFFSET 0xffffffff

// Describes one transform job. Mirrors Tr2TransformVerticesJob in trinity/Raytracing/Tr2RaytracingVertexTransforms.h,
// field for field.
// Offsets and strides are in bytes unless stated otherwise.
struct TransformJob
{
    uint VertexCount;
    uint VertexStride;
    // Index of the input vertex buffer in the buffer heap view array (DX12 only).
    uint InVBIndex;
    // Index of the first output vertex in the output buffer. Each output vertex is 3 floats.
    uint OutVBVertexOffset;

    // POSITION0 (FLOAT32_3). Includes the offset of the vertex allocation.
    uint PositionOffset;
    // NORMAL0 and its data type (FLOAT16_3 or FLOAT32_3), or TRANSFORM_VERTICES_INVALID_OFFSET.
    uint NormalOffset;
    uint NormalType;
    // TEXCOORD0 and its data type (FLOAT16_2 or FLOAT32_2), or TRANSFORM_VERTICES_INVALID_OFFSET.
    uint TexCoord0Offset;
    uint TexCoord0Type;

    // BLENDINDICES0 (4 bytes), or TRANSFORM_VERTICES_INVALID_OFFSET if the vertices aren't skinned.
    uint BoneOffset;
    // BLENDWEIGHTS0 (4 bytes, weight = byte / 255), or TRANSFORM_VERTICES_INVALID_OFFSET for one bone per vertex.
    uint BoneWeightsOffset;
    // Index of the mesh's first bone in BoneTransforms.
    uint TransformOffset;

    // The mesh's entries in MorphTargetAnimations.
    uint MorphAnimationDataOffset;
    uint MorphAnimationDataCount;
    // Position of the first vertex of morph target 0, in the input vertex buffer.
    uint MorphTargetPositionOffset;
    uint MorphTargetStride;
    // Size of one morph target (VertexCount * MorphTargetStride).
    uint MorphTargetSize;
    // Position of the first vertex in BakedMorphTargetBuffer, or TRANSFORM_VERTICES_INVALID_OFFSET.
    uint BakedMorphTargetPositionOffset;

    // Index of the job's first thread group in TransformJobGroups, relative to GroupBase.
    uint FirstGroup;
    uint Padding;
};

// All jobs of the frame.
StructuredBuffer<TransformJob> TransformJobs <bool AutoRegister = true; >;
// For each thread group: the index of its job, relative to JobBase.
StructuredBuffer<uint> TransformJobGroups <bool AutoRegister = true; >;

RWBuffer<uint> BakedMorphTargetBuffer <bool AutoRegister = true; >;

StructuredBuffer<float> InVB;
RWStructuredBuffer<float> OutVB;

// Per-dispatch constants. Not named PerObjectVS so that a material can include this file next to its own per-object data.
// Mirrors TransformVerticesDispatchCBuffer in Tr2RaytracingVertexTransforms.cpp.
cbuffer TransformVerticesDispatch : register( b3 )
{
    // Index of the frame's first job in TransformJobs.
    uint JobBase;
    // Index of the frame's first thread group in TransformJobGroups.
    uint GroupBase;
    // Index of the dispatch's first thread group, relative to GroupBase.
    uint DispatchFirstGroup;
    // Index of the output buffer in the buffer heap view array (DX12 only).
    uint OutVBIndex;
};

#if PLATFORM==PLATFORM_DX12

StructuredBuffer<float> HeapView_StructuredBuffer[]
<
    bool IsHeapView = true;
>;

RWStructuredBuffer<float> HeapView_RWStructuredBuffer[]
<
    bool IsHeapView = true;
>;

// Reads 4 bytes from the job's input vertex buffer. The address is in bytes and must be a multiple of 4.
uint LoadVertexBufferUInt( TransformJob job, uint address )
{
    // InVBIndex is the same for all threads of a thread group, but differs between the thread groups of a dispatch.
    return asuint( HeapView_StructuredBuffer[NonUniformResourceIndex( job.InVBIndex )][address >> 2] );
}

void StoreTransformedFloat( uint index, float value )
{
    HeapView_RWStructuredBuffer[OutVBIndex][index] = value;
}

#else

// Reads 4 bytes from the job's input vertex buffer. The address is in bytes and must be a multiple of 4.
// All jobs of a dispatch share the input vertex buffer bound to InVB.
uint LoadVertexBufferUInt( TransformJob job, uint address )
{
    return asuint( InVB[address >> 2] );
}

void StoreTransformedFloat( uint index, float value )
{
    OutVB[index] = value;
}

#endif

float LoadVertexBufferFloat( TransformJob job, uint address )
{
    return asfloat( LoadVertexBufferUInt( job, address ) );
}

float2 LoadVertexBufferFloat32_2( TransformJob job, uint address )
{
    return float2( LoadVertexBufferFloat( job, address ), LoadVertexBufferFloat( job, address + 4 ) );
}

float3 LoadVertexBufferFloat32_3( TransformJob job, uint address )
{
    return float3( LoadVertexBufferFloat( job, address ), LoadVertexBufferFloat( job, address + 4 ), LoadVertexBufferFloat( job, address + 8 ) );
}

// The address must be a multiple of 4.
float2 LoadVertexBufferFloat16_2( TransformJob job, uint address )
{
    return f16tof32( UnpackUInt16_2( LoadVertexBufferUInt( job, address ) ) );
}

// The address must be a multiple of 2.
float3 LoadVertexBufferFloat16_3( TransformJob job, uint address )
{
    uint alignedAddress = address & ~3u;
    return f16tof32( UnpackUInt16_3( LoadVertexBufferUInt( job, alignedAddress ), LoadVertexBufferUInt( job, alignedAddress + 4 ), address ) );
}

uint4 FromUByte4( uint x )
{
    return uint4( x & 0xff, ( x >> 8 ) & 0xff, ( x >> 16 ) & 0xff, x >> 24 );
}

// Finds the job and the vertex of a thread. Returns false if the thread has no vertex (the last thread group of a job
// can be partly empty). Pass SV_GroupID.x and SV_GroupThreadID.x.
bool GetTransformVertex( uint groupId, uint threadId, out TransformJob job, out uint vtx )
{
    // the thread group's index relative to GroupBase
    uint group = DispatchFirstGroup + groupId;
    job = TransformJobs[JobBase + TransformJobGroups[GroupBase + group]];
    vtx = ( group - job.FirstGroup ) * TRANSFORM_VERTICES_GROUP_SIZE + threadId;
    return vtx < job.VertexCount;
}

// Position of the vertex in the input vertex buffer, before morphing and skinning.
float3 LoadPosition( TransformJob job, uint vtx )
{
    return LoadVertexBufferFloat32_3( job, vtx * job.VertexStride + job.PositionOffset );
}

// Normal of the vertex in the input vertex buffer, before morphing and skinning. Returns 0 if the mesh has no NORMAL0
// in a supported format.
float3 LoadNormal( TransformJob job, uint vtx )
{
    if( job.NormalOffset == TRANSFORM_VERTICES_INVALID_OFFSET )
    {
        return 0.0.xxx;
    }
    uint address = vtx * job.VertexStride + job.NormalOffset;
    if( job.NormalType == FLOAT32_3 )
    {
        return LoadVertexBufferFloat32_3( job, address );
    }
    if( job.NormalType == FLOAT16_3 )
    {
        return LoadVertexBufferFloat16_3( job, address );
    }
    return 0.0.xxx;
}

// First texture coordinate of the vertex. Returns 0 if the mesh has no TEXCOORD0 in a supported format.
float2 LoadTexCoord0( TransformJob job, uint vtx )
{
    if( job.TexCoord0Offset == TRANSFORM_VERTICES_INVALID_OFFSET )
    {
        return 0.0.xx;
    }
    uint address = vtx * job.VertexStride + job.TexCoord0Offset;
    if( job.TexCoord0Type == FLOAT32_2 )
    {
        return LoadVertexBufferFloat32_2( job, address );
    }
    if( job.TexCoord0Type == FLOAT16_2 )
    {
        return LoadVertexBufferFloat16_2( job, address );
    }
    return 0.0.xx;
}

// Applies the animated morph targets and the baked morph target to a position loaded with LoadPosition.
float3 ApplyMorphs( TransformJob job, uint vtx, float3 pos )
{
    float3 basePos = pos;

    uint morphAddress = vtx * job.MorphTargetStride + job.MorphTargetPositionOffset;
    for( uint i = 0; i < job.MorphAnimationDataCount; i++ )
    {
        MorphTargetAnimationData data = MorphTargetAnimations[job.MorphAnimationDataOffset + i];
        float3 morphPos = LoadVertexBufferFloat32_3( job, morphAddress + job.MorphTargetSize * data.index );
        pos += data.weight * ( morphPos - basePos );
    }

    if( job.BakedMorphTargetPositionOffset != TRANSFORM_VERTICES_INVALID_OFFSET )
    {
        uint bakedAddress = job.BakedMorphTargetPositionOffset + vtx * job.MorphTargetStride;
        float3 morphPos = ReadFloat32_3( BakedMorphTargetBuffer, bakedAddress );
        pos += morphPos - basePos;
    }

    return pos;
}

// Transforms a position by the vertex's bones. Returns the position unchanged if the vertices aren't skinned.
float3 ApplySkinning( TransformJob job, uint vtx, float3 pos )
{
    if( job.BoneOffset == TRANSFORM_VERTICES_INVALID_OFFSET )
    {
        return pos;
    }

    uint boneIndices = LoadVertexBufferUInt( job, vtx * job.VertexStride + job.BoneOffset );

    float4x4 boneMatrix;
    if( job.BoneWeightsOffset != TRANSFORM_VERTICES_INVALID_OFFSET )
    {
        uint4 bones = FromUByte4( boneIndices );
        uint4 w = FromUByte4( LoadVertexBufferUInt( job, vtx * job.VertexStride + job.BoneWeightsOffset ) );
        float4 weights = float4( w ) / 255.0;

        boneMatrix =
            GetBoneTransform( job.TransformOffset + bones.x ) * weights.x +
            GetBoneTransform( job.TransformOffset + bones.y ) * weights.y +
            GetBoneTransform( job.TransformOffset + bones.z ) * weights.z +
            GetBoneTransform( job.TransformOffset + bones.w ) * weights.w;
    }
    else
    {
        boneMatrix = GetBoneTransform( job.TransformOffset + ( boneIndices & 0xff ) );
    }
    return mul( float4( pos, 1 ), boneMatrix ).xyz;
}

// Writes the final position of the vertex to the output buffer.
void StoreTransformedPosition( TransformJob job, uint vtx, float3 pos )
{
    uint index = ( job.OutVBVertexOffset + vtx ) * 3;
    StoreTransformedFloat( index, pos.x );
    StoreTransformedFloat( index + 1, pos.y );
    StoreTransformedFloat( index + 2, pos.z );
}

#endif
