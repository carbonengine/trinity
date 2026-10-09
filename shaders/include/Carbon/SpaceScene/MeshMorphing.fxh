// Copyright © 2026 CCP ehf.

#ifndef CARBON_MESHMORPHING_FXH
#define CARBON_MESHMORPHING_FXH


#include "../VertexBufferUtil.fxh"

Buffer<uint> SharedIndexVertexBuffer <bool AutoRegister = true; >;

struct MorphTargetAnimationData
{
    uint index;
    float weight;
};
StructuredBuffer<MorphTargetAnimationData> MorphTargetAnimations <bool AutoRegister = true; >;

// the following assumes that all morphed objects will be preprocessed and hence packed
// it also assumes no delta compression is being used
#define MORPH_TARGERT_CONSTANT_SIZE 24
struct MorphTargetGeometryConstants
{
    uint vertexBufferStride;
    uint positionOffset;
    uint positionType;
    uint tangentOffset;
    uint tangentType;
    uint vertexCount;
};

MorphTargetGeometryConstants GetMorphTargetGeometryConstants( uint4 MorphTargetData )
{
    MorphTargetGeometryConstants constants;
    constants.vertexBufferStride = ReadUInt32( SharedIndexVertexBuffer, MorphTargetData.x + 0 );
    constants.positionOffset = ReadUInt32( SharedIndexVertexBuffer, MorphTargetData.x + 4 );
    constants.positionType = ReadUInt32( SharedIndexVertexBuffer, MorphTargetData.x + 8 );
    constants.tangentOffset = ReadUInt32( SharedIndexVertexBuffer, MorphTargetData.x + 12 );
    constants.tangentType = ReadUInt32( SharedIndexVertexBuffer, MorphTargetData.x + 16 );
    constants.vertexCount = ReadUInt32( SharedIndexVertexBuffer, MorphTargetData.x + 20 );

    return constants;
}

void ProcessMorphs( MorphTargetGeometryConstants constants, uint vertexID, uint4 MorphTargetData, inout float3 vtxPos, inout float4 vtxPackedTangent )
{
    float normalSign = vtxPackedTangent.w;
    vtxPackedTangent.w = sqrt( saturate( 1. - dot( vtxPackedTangent.xyz, vtxPackedTangent.xyz ) ) );

    int dataOffset = MORPH_TARGERT_CONSTANT_SIZE; // MorphTargetGeometryConstants size
    dataOffset += MorphTargetData.x; // offset in shared index/vertex buffer
    dataOffset += vertexID * constants.vertexBufferStride;

    int posOffset = dataOffset + constants.positionOffset;
    int tangentOffset = dataOffset + constants.tangentOffset;
    
    int morphTargetSize = constants.vertexCount * constants.vertexBufferStride;

    float3 tempPos = vtxPos;
    float4 tempPackedTangent = vtxPackedTangent;
    for( uint i = 0; i < MorphTargetData.z; i++ )
    {
        MorphTargetAnimationData data = MorphTargetAnimations[MorphTargetData.y + i];
        int morphTargetIndex = data.index;
        int offset = morphTargetSize * morphTargetIndex;

        float3 pos = ReadFloat32_3( SharedIndexVertexBuffer, posOffset + offset );
        float4 packedTangent = float4( ReadInt16_4( SharedIndexVertexBuffer, tangentOffset + offset ) );
        packedTangent = max( packedTangent * ( 1. / 32767.0f ), float4( -1.0f, -1.0f, -1.0f, -1.0f ) );
        packedTangent.w = sqrt( saturate( 1. - dot( packedTangent.xyz, packedTangent.xyz ) ) );
        
        [flatten]
        if( dot( packedTangent, tempPackedTangent ) < 0. )
        {
            packedTangent = -packedTangent;
        }

        vtxPos += data.weight * ( pos - tempPos );
        vtxPackedTangent += data.weight * ( packedTangent - tempPackedTangent );
    }
    vtxPackedTangent = normalize( vtxPackedTangent );

    [flatten]
    if( vtxPackedTangent.w < 0. )
    {
        vtxPackedTangent = -vtxPackedTangent;
    }

    vtxPackedTangent.w = normalSign;
}

void ProcessMorphs( MorphTargetGeometryConstants constants, uint vertexID, uint4 MorphTargetData, inout float3 vtxPos )
{
    int dataOffset = MORPH_TARGERT_CONSTANT_SIZE; // MorphTargetGeometryConstants size
    dataOffset += MorphTargetData.x; // offset in shared index/vertex buffer
    dataOffset += vertexID * constants.vertexBufferStride;

    int posOffset = dataOffset + constants.positionOffset;
    
    int morphTargetSize = constants.vertexCount * constants.vertexBufferStride;

    float3 tempPos = vtxPos;
    for( uint i = 0; i < MorphTargetData.z; i++ )
    {
        MorphTargetAnimationData data = MorphTargetAnimations[MorphTargetData.y + i];
        int morphTargetIndex = data.index;
        int offset = morphTargetSize * morphTargetIndex;
        float3 pos = ReadFloat32_3( SharedIndexVertexBuffer, posOffset + offset );
        vtxPos += data.weight * ( pos - tempPos );
    }
}


#endif