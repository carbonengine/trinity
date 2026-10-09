// Copyright © 2026 CCP ehf.

#pragma permutation(FILL_MODE, values=(FILL_SOLID, FILL_WIREFRAME, FILL_NORMALS, FILL_TANGENTS), default=FILL_WIREFRAME)
#pragma permutation(COLOR_MODE, values=(COLOR_SOLID, COLOR_LIT, COLOR_UV0, COLOR_UV1, COLOR_VERTEX_COLOR))
#pragma permutation(TANGENT_PACKING, values=(TANGENT_NONE, TANGENT_PACKED, TANGENT_UNPACKED, TANGENT_NORMALS))

#include "../include/Carbon/System.fxh"
#include "../include/Carbon/Math.fxh"
#include "../include/Carbon/PackedTangents.fxh"

struct VSIn
{
    float3 position : POSITION;
#if TANGENT_PACKING == TANGENT_PACKED
    float4 tangents : TANGENT;
#elif TANGENT_PACKING == TANGENT_UNPACKED
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
#elif TANGENT_PACKING == TANGENT_NORMALS
	float3 normal : NORMAL;
#endif

#if COLOR_MODE == COLOR_UV0
    float2 texCoord : TEXCOORD0;
#elif COLOR_MODE == COLOR_UV1
	float2 texCoord : TEXCOORD1;
#elif COLOR_MODE == COLOR_VERTEX_COLOR
	float4 color : COLOR;
#endif
};

struct VSOut
{
    float4 position : SV_Position;
    float4 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
    float3 modelPos : TEXCOORD3;
};

struct PerFrameVSData
{
    float4x4 ViewInverseTransposeMat;
    float4x4 ViewProjectionMat;
} PerFrameVS : register( PERFRAME_VS_STARTREGISTER );


TangentSpace UnpackTangentSpace( VSIn inVtx )
{
    TangentSpace ts;
#if TANGENT_PACKING == TANGENT_PACKED
    ts = UnpackTangentSpaceLegacy( inVtx.tangents );
#elif TANGENT_PACKING == TANGENT_NORMALS
	ts.normal = inVtx.normal;
	ts.tangent = 0;
	ts.bitangent = 0;
#elif TANGENT_PACKING == TANGENT_UNPACKED
	ts.normal = inVtx.normal;
	ts.tangent = inVtx.tangent;
	ts.bitangent = inVtx.binormal;
#else
	ts.normal = 1;
	ts.tangent = 1;
	ts.bitangent = 1;
#endif
    return ts;
}

VSOut VS( VSIn inVtx )
{
    VSOut outVtx;
    outVtx.position = mul( float4( inVtx.position, 1 ), PerFrameVS.ViewProjectionMat );
    outVtx.normal = 1;
    outVtx.tangent = 1;
    outVtx.bitangent = 1;
    outVtx.modelPos = inVtx.position;
#if COLOR_MODE == COLOR_LIT || FILL_MODE == FILL_NORMALS || FILL_MODE == FILL_TANGENTS
    TangentSpace ts = UnpackTangentSpace( inVtx );
    outVtx.normal.xyz = ts.normal;
    outVtx.tangent = ts.tangent;
    outVtx.bitangent = ts.bitangent;
#elif COLOR_MODE == COLOR_UV0 || COLOR_MODE == COLOR_UV1
	outVtx.normal.xy = inVtx.texCoord;
#elif COLOR_MODE == COLOR_VERTEX_COLOR
	outVtx.normal = inVtx.color;
#endif
    return outVtx;
}

void AddVector( VSOut vertex, float3 direction, float4 color, inout LineStream<VSOut> triStream )
{
    float4 p0 = mul( float4( vertex.modelPos, 1 ), PerFrameVS.ViewProjectionMat );
    float length = p0.z;
    float4 p1 = mul( float4( vertex.modelPos + normalize( direction ) * length, 1 ), PerFrameVS.ViewProjectionMat );

    VSOut p = (VSOut)0;
    p.normal = color;

    triStream.RestartStrip();

    p.position = p0;
    triStream.Append( p );

    p.position = p1;
    triStream.Append( p );
}

[maxvertexcount( 18 )]
void GS( triangle VSOut inVtx[3], inout LineStream<VSOut> triStream, uint primitiveID : SV_PrimitiveID )
{
    for( int i = 0; i < 3; ++i )
    {
        VSOut vertex = inVtx[i];
        AddVector( vertex, vertex.normal.xyz, float4( 0, 0, 1, 1 ), triStream );
#if FILL_MODE == FILL_TANGENTS
        AddVector( vertex, vertex.tangent, float4( 1, 0, 0, 1 ), triStream );
        AddVector( vertex, vertex.bitangent, float4( 0, 1, 0, 1 ), triStream );
#endif
    }
}

float4 SolidPS( VSOut inVtx ) : SV_Target
{
    return inVtx.normal;
}

float4 PS( VSOut inVtx ) : SV_Target
{
#if COLOR_MODE == COLOR_LIT
    return saturate( dot( normalize( inVtx.normal.xyz ), normalize( float3( 1, 1, 1 ) ) ) ) * 0.9 + 0.1;
#elif COLOR_MODE == COLOR_UV0 || COLOR_MODE == COLOR_UV1
	return float4( inVtx.normal.xy, 0, 1 );
#elif COLOR_MODE == COLOR_VERTEX_COLOR
	return inVtx.normal;
#endif
    return 1;
}

technique Main
{
#if FILL_MODE == FILL_WIREFRAME
    pass p0
    {
        FillMode = Solid;
        ColorWriteEnable = 0;

        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
    pass p1
    {
        ColorWriteEnable = 0xf;
        FillMode = Wireframe;
        SlopeScaleDepthBias = 0.5;

        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
#elif (FILL_MODE == FILL_TANGENTS || FILL_MODE == FILL_NORMALS) && PLATFORM_SUPPORTS_GEOMETRY_SHADERS
	pass p1
	{
		CullMode = None;
		VertexShader = compile vs_3_0 VS();
		GeometryShader = compile gs_5_0 GS();
		PixelShader = compile ps_3_0 SolidPS();
	}
#else
	pass p1
	{
		VertexShader = compile vs_3_0 VS();
		PixelShader = compile ps_3_0 PS();
	}
#endif
}