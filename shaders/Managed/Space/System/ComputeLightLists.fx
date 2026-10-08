// Copyright © 2026 CCP ehf.

#include "../../../include/Carbon/System.fxh"

#define TILE_WIDTH 16
#define TILE_HEIGHT 16

Texture2D<float> DepthMap;

struct PerLightData
{
    float4 position; // xyz - world position, w - radius
    float4 color; // xyz - color, w -  bits 0..15 - innerRadius (float 16), bits 16..31 - flags
    float4 direction; // see next few lines:
    // x bits 0..15 - direction.x (float 16), x bits 16..31 - direction.y (float 16)
    // y bits 0..15 - direction.z (float 16), y bits 16..31 - projectionPlaneDistance (float 16)
    // z bits 0..15 - outerAngle (float 16), z bits 16..31 - innerAngle (float 16)
    // w bits 0..1 - padding, w bits 2..11 - shadowMapScale, w bits 12..21 - shadowMapOffsetX, w bits 22..31 - shadowMapOffsetY
};
static const uint LIGHT_FLAG_AFFECTS_PARTICLES = 1 << 17;



StructuredBuffer<PerLightData> LightBuffer;
RWStructuredBuffer<uint> LightIndices : register( u1 ) <
	// These annotations are used by Tr2LightManager
	int TileWidth = TILE_WIDTH;
	int TileHeight = TILE_HEIGHT;
	>;
RWBuffer<int> LightIndexCount;

struct
{
    float4x4 ViewInverse;
    float4x4 ProjInverse;
    float4 CameraPosition;
    uint4 FrameBufferSizes; // xy - frame buffer size; zw - tiles per width and height
    uint LightCount; // number of lights in LightBuffer
    uint IndexBufferSize; // number of elements in LightIndices 
} PerFramePS : register( c200 );

float3 ComputeWorldPos( float2 pixel, float depth, float2 wh )
{
    float2 uv = pixel / wh;
    float2 clipPos = uv * 2 - 1;
    clipPos.y = -clipPos.y;

    float4 pos = float4( clipPos, depth, 1 );
    pos = mul( pos, PerFramePS.ProjInverse );
    pos /= pos.w;
    pos = mul( pos, PerFramePS.ViewInverse );
    return pos.xyz;
}

float GetViewDepth( float depth )
{
    float4 p = mul( float4( 0, 0, depth, 1 ), PerFramePS.ProjInverse );
    return p.z / p.w;
}

float ConeSphereIntersect( float3 conePosition, float3 coneDirection, float sinAngle, float3 center, float radius )
{
    float3 d = center - conePosition;
    float c = length( d );
    d = normalize( d );
    float ca = dot( d, coneDirection );
    float a = c * ca;
    float b = sqrt( c * c - a * a );

    return saturate( radius + sinAngle * c - b );
}


struct Frustum
{
    float3 coneDirection;
    float coneAngleSine;
    float3 cameraDirection;
	// near/far planes constained to depth buffer contents (for opaque lists)
    float nearDepth;
    float farDepth;
	// near/far planes from the camera (for transparent lists)
    float nearPlane;
    float farPlane;
};

void CreateFrustum( out Frustum frustum, float2 tile, float2 depths, float transparentDepth )
{
    float2 screenSize = (float2)PerFramePS.FrameBufferSizes.xy;
    float minD = depths.x;
    float maxD = depths.y;

	//	near, far
    float minDV = GetViewDepth( minD );
    float maxDV = GetViewDepth( maxD );
    float3 viewDir = mul( float4( 0, 0, -1, 0 ), PerFramePS.ViewInverse ).xyz;

    frustum.cameraDirection = viewDir;
    frustum.nearDepth = dot( PerFramePS.ViewInverse[3].xyz, -viewDir ) + minDV;
    frustum.farDepth = dot( PerFramePS.ViewInverse[3].xyz, -viewDir ) + maxDV;
    frustum.nearPlane = dot( PerFramePS.ViewInverse[3].xyz, -viewDir ) + GetViewDepth( 0 );
    frustum.farPlane = dot( PerFramePS.ViewInverse[3].xyz, -viewDir ) + GetViewDepth( transparentDepth );

    float left = tile.x * TILE_WIDTH;
    float top = tile.y * TILE_HEIGHT;
    float right = min( screenSize.x, left + TILE_WIDTH );
    float bottom = min( screenSize.y, top + TILE_HEIGHT );

    float3 ful = ComputeWorldPos( float2( left, top ), maxD, screenSize );
    float3 fbr = ComputeWorldPos( float2( right, bottom ), maxD, screenSize );

    float3 c = PerFramePS.CameraPosition.xyz;

    float3 r0 = normalize( ful - c );
    float3 r1 = normalize( fbr - c );

    float3 dir = normalize( r0 + r1 );
    frustum.coneDirection.xyz = dir;
    frustum.coneAngleSine = sqrt( 1 - dot( dir, r0 ) * dot( dir, r0 ) );
}

bool SphereInFrustum( float3 center, float radius, Frustum frustum, out bool transparent )
{
	// check depth: near/far planes
    float inside = 1;

    inside *= ConeSphereIntersect( PerFramePS.CameraPosition.xyz, frustum.coneDirection, frustum.coneAngleSine, center, radius );

    float transparentInside = inside;

    inside *= saturate( radius - dot( float4( center, 1 ), float4( -frustum.cameraDirection, -frustum.nearDepth ) ) );
    inside *= saturate( radius - dot( float4( center, 1 ), float4( frustum.cameraDirection, frustum.farDepth ) ) );

    transparentInside *= saturate( radius - dot( float4( center, 1 ), float4( -frustum.cameraDirection, -frustum.nearPlane ) ) );
    transparentInside *= saturate( radius - dot( float4( center, 1 ), float4( frustum.cameraDirection, frustum.farPlane ) ) );

    transparent = inside <= 0;

    return inside > 0 || transparentInside > 0;
}

bool ConeInFrustum( float3 center, float radius, float3 spotlightDir, float spotlightAngleCos, Frustum frustum, out bool transparent )
{
	// check first for a sphere that the cone is a subsection of. 
    float inside = ConeSphereIntersect( PerFramePS.CameraPosition.xyz, frustum.coneDirection, frustum.coneAngleSine, center, radius );
	
	// this is a very gross approxomation of the spotlight occlusion...
    float cosAngle = spotlightAngleCos;
    float hypotenuse = radius / cosAngle;
    float sphereRadius = 0.5f * hypotenuse / cosAngle;
    float3 sphereCenter = clamp( 0.0, radius, sphereRadius ) * normalize( -spotlightDir );

    inside *= ConeSphereIntersect( PerFramePS.CameraPosition.xyz, frustum.coneDirection, frustum.coneAngleSine, center + sphereCenter, sphereRadius );

    float transparentInside = inside;

    inside *= saturate( radius - dot( float4( center, 1 ), float4( -frustum.cameraDirection, -frustum.nearDepth ) ) );
    inside *= saturate( radius - dot( float4( center, 1 ), float4( frustum.cameraDirection, frustum.farDepth ) ) );

    transparentInside *= saturate( radius - dot( float4( center, 1 ), float4( -frustum.cameraDirection, -frustum.nearPlane ) ) );
    transparentInside *= saturate( radius - dot( float4( center, 1 ), float4( frustum.cameraDirection, frustum.farPlane ) ) );

    transparent = inside <= 0;

    return inside > 0 || transparentInside > 0;
}


groupshared uint g_tileHeadIndex;
groupshared uint g_tileTransparentHeadIndex;
groupshared uint g_tileParticleHeadIndex;
groupshared uint g_tileTransparentTailIndex;


void AddLight( uint light, uint heapOffset, uint indexBufferSize, bool transparent )
{
    int iHead;
    InterlockedAdd( LightIndexCount[0], 1, iHead );
    uint head = iHead;

    head = head * 2 + heapOffset;
    if( head < indexBufferSize )
    {
        if( transparent )
        {
            uint next;
            InterlockedExchange( g_tileTransparentHeadIndex, head, next );

            LightIndices[head] = light;
            LightIndices[head + 1] = next;

            if( next == 0 )
            {
                g_tileTransparentTailIndex = head + 1;
            }
        }
        else
        {
            uint next;
            InterlockedExchange( g_tileHeadIndex, head, next );

            LightIndices[head] = light;
            LightIndices[head + 1] = next;
        }
    }
}

groupshared uint g_DepthMinShared;
groupshared uint g_DepthMaxShared;
groupshared uint g_DepthMaxTransparentShared;
groupshared Frustum g_frustum;


[numthreads( TILE_WIDTH, TILE_HEIGHT, 1 )]
void ComputeLightLists( uint3 nGid : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 offset : SV_DispatchThreadID )
{
    if( groupIndex == 0 )
    {
        g_DepthMinShared = asuint( 1.0 );
        g_DepthMaxShared = asuint( 0.0 );
        g_DepthMaxTransparentShared = asuint( 0.0 );
    }

    GroupMemoryBarrierWithGroupSync();

    float minD = 1;
    float minDD = 1;
    float maxD = 0;
    float d = DepthMap[offset.xy];
    minDD = min( d, minDD );
    if( d != 0 )
    {
        minD = maxD = d;
    }
    if( maxD != 0 )
    {
		// have to add epsilon values to depth range because of precision issues when depth buffer range is too large
        minD = max( 0.0, minD - 1.19e-07 );
        minDD = max( 0.0, minDD - 1.19e-07 );
        maxD = min( 1.0, maxD + 1.19e-07 );
    }
    InterlockedMin( g_DepthMinShared, asuint( 1 - maxD ) );
    InterlockedMax( g_DepthMaxShared, asuint( 1 - minD ) );
    InterlockedMax( g_DepthMaxTransparentShared, asuint( 1 - minDD ) );

    GroupMemoryBarrierWithGroupSync();

    if( groupIndex == 0 )
    {
        float2 tileDepth = float2( asfloat( g_DepthMinShared ), asfloat( g_DepthMaxShared ) );
        float tileTransparentDepth = asfloat( g_DepthMaxTransparentShared );

        Frustum frustum;
        CreateFrustum( frustum, (float2)nGid.xy, tileDepth, tileTransparentDepth );
        g_frustum = frustum;
        g_tileHeadIndex = 0;
        g_tileTransparentHeadIndex = 0;
        g_tileTransparentTailIndex = 0;
        g_tileParticleHeadIndex = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint numLights = PerFramePS.LightCount;
    uint heapOffset = PerFramePS.FrameBufferSizes.z * PerFramePS.FrameBufferSizes.w * 3;
    uint indexBufferSize = PerFramePS.IndexBufferSize;

    for( uint light = groupIndex; light < numLights; light += TILE_WIDTH * TILE_HEIGHT )
    {
        PerLightData lightData = LightBuffer[light];
        float4 lightPos = lightData.position;
        float3 lightDir = float3(
			f16tof32( asuint( lightData.direction.x ) & 0xffff ),
			f16tof32( asuint( lightData.direction.x ) >> 16 ),
			f16tof32( asuint( lightData.direction.y ) & 0xffff )
		);
        float spotlightAngleCos = f16tof32( asuint( lightData.direction.z ) & 0xffff );

        bool inside = false;
        bool transparent = false;

        if( spotlightAngleCos <= 0 )
        {
            if( SphereInFrustum( lightPos.xyz, lightPos.w, g_frustum, transparent ) )
            {
                inside = true;
            }
        }
        else
        {
            if( ConeInFrustum( lightPos.xyz, lightPos.w, lightDir.xyz, spotlightAngleCos, g_frustum, transparent ) )
            {
                inside = true;
            }
        }
        if( inside )
        {
            AddLight( light, heapOffset, indexBufferSize, transparent );
            uint lightFlags = asuint( lightData.color.w );
            if( lightFlags & LIGHT_FLAG_AFFECTS_PARTICLES )
            {
                int iHead;
                InterlockedAdd( LightIndexCount[0], 1, iHead );
                uint head = iHead;

                head = head * 2 + heapOffset;
                if( head < indexBufferSize )
                {
                    uint next;
                    InterlockedExchange( g_tileParticleHeadIndex, head, next );

                    LightIndices[head] = light;
                    LightIndices[head + 1] = next;
                }
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if( groupIndex == 0 )
    {
        uint linearTile = ( nGid.x + nGid.y * PerFramePS.FrameBufferSizes.z ) * 3;
        LightIndices[linearTile] = g_tileHeadIndex;
        LightIndices[linearTile + 1] = g_tileTransparentHeadIndex ? g_tileTransparentHeadIndex : g_tileHeadIndex;
        LightIndices[linearTile + 2] = g_tileParticleHeadIndex;
        if( g_tileTransparentTailIndex != 0 )
        {
            LightIndices[g_tileTransparentTailIndex] = g_tileHeadIndex;
        }
    }
}

[numthreads( 1, 1, 1 )]
void ClearLightListCounter()
{
    LightIndexCount[0] = 0;
}

technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 ComputeLightLists();
    }
}


technique Clear
{
    pass p0
    {
        ComputeShader = compile cs_5_0 ClearLightListCounter();
    }
}
