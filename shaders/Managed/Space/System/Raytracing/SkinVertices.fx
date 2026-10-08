// Copyright © 2026 CCP ehf.

#include "../../../../include/Carbon/System.fxh"
#include "../../../../include/Carbon/BoneTransforms.fxh"
#include "../../../../include/Carbon/VertexBufferUtil.fxh"

StructuredBuffer<float> InVB;
RWStructuredBuffer<float> OutVB;

struct AnimationData
{
    uint index;
    float weight;
};
StructuredBuffer<AnimationData> MorphTargetAnimations <bool AutoRegister = true; >;
RWBuffer<uint> BakedMorphTargetBuffer <bool AutoRegister = true; >;

cbuffer PerObjectVS : register( b3 )
{
    uint VertexCount;
    uint Stride;
    uint PositionOffset;
    uint BoneOffset;
    uint BoneWeightOffset;
    uint TransformOffset;
    uint InVBIndex;
    uint OutVBIndex;
    uint OutVBOffset;
    uint MorphAnimationDataOffset;
    uint MorphAnimationDataCount;
    uint MorphTargetPositionOffset;
    uint MorphTargetStride;
    uint MorphTargetSize;
    uint BakedMorphTargetPositionOffset;
    uint Padding2;
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

float LoadVB( uint offset )
{
	return HeapView_StructuredBuffer[InVBIndex][offset];
}

void StoreVB( uint offset, float value )
{
	HeapView_RWStructuredBuffer[OutVBIndex][offset + OutVBOffset] = value;
}

#else

float LoadVB( uint offset )
{
    return InVB[offset];
}

void StoreVB( uint offset, float value )
{
    OutVB[offset + OutVBOffset] = value;
}

#endif

uint4 FromUByte4( uint x )
{
    return uint4( x & 0xff, ( x >> 8 ) & 0xff, ( x >> 16 ) & 0xff, x >> 24 );
}

[numthreads( 64, 1, 1 )]
void SkinVertices( uint3 offset : SV_DispatchThreadID )
{
    uint vtx = offset.x;
	
    if( vtx < VertexCount )
    {
        uint poffset = ( vtx * Stride + PositionOffset );
        float3 pos = float3( LoadVB( poffset ), LoadVB( poffset + 1 ), LoadVB( poffset + 2 ) );
		
        int morphOffset = vtx * MorphTargetStride + MorphTargetPositionOffset;
        float3 tmpPos = pos;
        for( uint i = 0; i < MorphAnimationDataCount; i++ )
        {
            AnimationData data = MorphTargetAnimations[MorphAnimationDataOffset + i];
            int offset = morphOffset + MorphTargetSize * data.index;
            float3 morphPos = float3( LoadVB( offset ), LoadVB( offset + 1 ), LoadVB( offset + 2 ) );
            pos += data.weight * ( morphPos - tmpPos );
        }

        if( BakedMorphTargetPositionOffset != 0xFFFFFFFF )
        {
            int bakedPosOffset = BakedMorphTargetPositionOffset; // offset in shared index/vertex buffer, but for baked morphs
            bakedPosOffset += ( vtx * MorphTargetStride ) << 2;
			
            float3 morphPos = ReadFloat32_3( BakedMorphTargetBuffer, bakedPosOffset );

            pos += morphPos - tmpPos;
        }

        uint boffset = vtx * Stride + BoneOffset;

        float4x4 boneMatrix;
        uint boneIndices = asuint( LoadVB( boffset ) );
        if( BoneWeightOffset != 0xffffffff )
        {
            uint4 bones = FromUByte4( boneIndices );
            uint4 w = FromUByte4( asuint( LoadVB( vtx * Stride + BoneWeightOffset ) ) );
            float4 weights = float4( float4( w ) / 255.0 );

            boneMatrix =
				GetBoneTransform( TransformOffset + bones.x ) * weights.x +
				GetBoneTransform( TransformOffset + bones.y ) * weights.y +
				GetBoneTransform( TransformOffset + bones.z ) * weights.z +
				GetBoneTransform( TransformOffset + bones.w ) * weights.w;
        }
        else
        {
            boneMatrix = GetBoneTransform( TransformOffset + ( boneIndices & 0xff ) );
        }
        pos = mul( float4( pos, 1 ), boneMatrix ).xyz;
        StoreVB( vtx * 3, pos.x );
        StoreVB( vtx * 3 + 1, pos.y );
        StoreVB( vtx * 3 + 2, pos.z );
    }
}

technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 SkinVertices();
    }
}
