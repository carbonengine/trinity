// Copyright © 2026 CCP ehf.

#pragma permutation(TEXTURE_TYPE, values=(TEXTURE_2D, TEXTURE_CUBE, TEXTURE_3D), default=TEXTURE_2D, description="Texture type")
#pragma permutation(TEXTURE_MSAA, values=(MSAA_NO, MSAA_YES), default=MSAA_NO, description="Texture MSAA type")
#pragma permutation(TEXTURE_ARRAY, values=(ARRAY_NO, ARRAY_YES), default=ARRAY_NO, description="Texture array type")
#pragma permutation(TEXTURE_FORMAT, values=(FORMAT_FLOAT, FORMAT_INT, FORMAT_UINT))
#pragma permutation(COLOR_SPACE, values = ( LINEAR_SPACE, SRGB_SPACE, LOGARITHMIC_SPACE ) )
#pragma permutation(CUBE_RENDERING_MODE, values=(VIEW, CROSS, ROWS, FORWARD, BACKWARD, RIGHT, LEFT, UP, DOWN), default=VIEW, description="Cube Texture Rendering Mode")

#include "../include/Carbon/System.fxh"


// TODO: Support metal and CubeArrays
#if PLATFORM == PLATFORM_METAL
#define TEXTURE_ARRAY ARRAY_NO
#endif

#if TEXTURE_ARRAY == ARRAY_NO

#if TEXTURE_TYPE == TEXTURE_2D
#if TEXTURE_MSAA == MSAA_NO
#define _TEXTURE_TYPE Texture2D
#else // TEXTURE_MSAA == MSAA_NO
#define _TEXTURE_TYPE Texture2DMS
#endif // TEXTURE_MSAA == MSAA_NO
#elif TEXTURE_TYPE == TEXTURE_CUBE
#define _TEXTURE_TYPE TextureCube
#else // TEXTURE_TYPE
#define _TEXTURE_TYPE Texture3D
#endif // TEXTURE_TYPE

#else // TEXTURE_ARRAY

#if TEXTURE_TYPE == TEXTURE_2D
#if TEXTURE_MSAA == MSAA_NO
#define _TEXTURE_TYPE Texture2DArray
#else // TEXTURE_MSAA == MSAA_NO
#define _TEXTURE_TYPE Texture2DMSArray
#endif // TEXTURE_MSAA == MSAA_NO
#elif TEXTURE_TYPE == TEXTURE_CUBE
#define _TEXTURE_TYPE TextureCubeArray
#else // TEXTURE_TYPE
// There is no 3DArray
#define _TEXTURE_TYPE Texture3D
#endif // TEXTURE_TYPE

#endif // TEXTURE_ARRAY

#if TEXTURE_FORMAT == FORMAT_INT
#define _TEXTURE_FORMAT <int4>
#elif TEXTURE_FORMAT == FORMAT_UINT
#define _TEXTURE_FORMAT <uint4>
#else
#define _TEXTURE_FORMAT <float4>
#endif

_TEXTURE_TYPE _TEXTURE_FORMAT Texture;

float4 ColorTransform0;
float4 ColorTransform1;
float4 ColorTransform2;
float4 ColorTransform3;
float4 MinValue;
float4 MaxValue;
float UseForceMipMap;
float ForceMipMap;
float VolumeSlice = 0;
float ArraySlice = 0;


float4x4 ViewMat;

SamplerState AutoMipMapTexture
{
    AddressU = Border;
    AddressV = Border;
    AddressW = Clamp;
    BorderColor = 0;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

SamplerState NoMipMapTexture
{
    AddressU = Border;
    AddressV = Border;
    AddressW = Clamp;
    BorderColor = 0;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
};

struct BlitAppVertex
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct BlitVertex
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};


BlitVertex BlitVS( BlitAppVertex inVtx )
{
    BlitVertex outVtx = { inVtx.pos, inVtx.texCoord };
    return outVtx;
}

float4 LinearToSRGB( float4 inColor )
{
    float4 result;
    result.r = inColor.r < 0.0031308 ? inColor.r * 12.92 : pow( saturate( inColor.r ), 1 / 2.4 ) * 1.055 - 0.055;
    result.g = inColor.g < 0.0031308 ? inColor.g * 12.92 : pow( saturate( inColor.g ), 1 / 2.4 ) * 1.055 - 0.055;
    result.b = inColor.b < 0.0031308 ? inColor.b * 12.92 : pow( saturate( inColor.b ), 1 / 2.4 ) * 1.055 - 0.055;
    result.a = inColor.w;
    return result;
}

float4 SRGBToLinear( float4 inColor )
{
    float4 result;
    result.r = inColor.r < 0.040449936 ? inColor.r / 12.92 : pow( abs( ( inColor.r + 0.055 ) / 1.055 ), 2.4 );
    result.g = inColor.g < 0.040449936 ? inColor.g / 12.92 : pow( abs( ( inColor.g + 0.055 ) / 1.055 ), 2.4 );
    result.b = inColor.b < 0.040449936 ? inColor.b / 12.92 : pow( abs( ( inColor.b + 0.055 ) / 1.055 ), 2.4 );
    result.a = inColor.a < 0.040449936 ? inColor.a / 12.92 : pow( abs( ( inColor.a + 0.055 ) / 1.055 ), 2.4 );
    return result;
}

float4 LogarithmicToLinearish( float4 inColor )
{
	// Avert your eyes! Extreme HACK ahead!
    return pow( abs( inColor ), float4( 0.25, 0.25, 0.25, 0.25 ) );
}


#if TEXTURE_TYPE == TEXTURE_CUBE && TEXTURE_ARRAY == ARRAY_NO

static const float3x3 POS_X_ROTATION = float3x3( 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0 );
static const float3x3 NEG_X_ROTATION = float3x3( 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0 );
static const float3x3 POS_Z_ROTATION = float3x3( -1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, -1.0 );
static const float3x3 NEG_Z_ROTATION = float3x3( 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 );
static const float3x3 POS_Y_ROTATION = float3x3( -1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, -1.0, 0.0 );
static const float3x3 NEG_Y_ROTATION = float3x3( -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 );
static const float3x3 ERROR_ROTATION = float3x3( 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 );

float3 ConvertUvToViewDir( float2 uv )
{
    float2 pos = uv * 2 - 1;
    return float3( pos.x, -pos.y, -1 );
}
// returns the correct 3d view direction
float3 GetView( float2 uv, float2 columnRow, float2 size, float3x3 rotation )
{
    uv -= columnRow * size;
    uv /= size;
	// rotate the view direction 
    return mul( rotation, ConvertUvToViewDir( uv ) );
}
// returns the column and row for a specific grid size
float2 GetColumnRow( float2 uv, float columns, float rows )
{
    return float2( floor( uv.x * columns ), floor( uv.y * rows ) );
}
// moves a float value to a new min max 
float MoveValueToRange( float2 oldMinMax, float2 newMinMax, float value )
{
    return newMinMax.x + ( newMinMax.y - newMinMax.x ) / ( oldMinMax.y - oldMinMax.x ) * ( value - oldMinMax.x );
}
// returns a 
float4 CubeColor( float2 uv )
{
    float3 viewDir;
#if CUBE_RENDERING_MODE == CROSS
	//	     up
	//  left forward right back
	//       down
    float gutter = 1.0 / 8.0;
    if( uv.y < gutter || uv.y > 1.0 - gutter )
    {
		// gutter
        return float4( 0, 0, 0, 0 );
    }
	// rejig the uv to go from float2([0-1], [gutter - (1 - gutter)]) to float2([0-1], [0-1])
    float2 transformedUv = float2( uv.x, MoveValueToRange( float2( gutter, 1.0 - gutter ), float2( 0, 1 ), uv.y ) );
    float3x3 ROTATION_MATRICES[12] =
    {
        ERROR_ROTATION, POS_Y_ROTATION, ERROR_ROTATION, ERROR_ROTATION,
			NEG_X_ROTATION, POS_Z_ROTATION, POS_X_ROTATION, NEG_Z_ROTATION,
			ERROR_ROTATION, NEG_Y_ROTATION, ERROR_ROTATION, ERROR_ROTATION
    };
    float2 gridSize = float2( 4.0, 3.0 );
    float2 columnRow = GetColumnRow( transformedUv, gridSize.x, gridSize.y );
			
    float3x3 rotation = ROTATION_MATRICES[columnRow.x + 4 * columnRow.y];
	// check the dead zone
    if( !any( mul( rotation, float3( 1, 1, 1 ) ) ) )
    {
        return float4( 0, 0, 0, 0 );
    }
    viewDir = GetView( transformedUv, columnRow, 1.0 / gridSize, rotation );
#elif CUBE_RENDERING_MODE == ROWS
	// recalculate the direction to show all rows
	//	left    right   up 
	// 	forward back    down
	float gutter = 1.0 / 6.0;
	if(uv.y < gutter || uv.y > 1.0 - gutter )
	{
		// gutter
		return float4(0, 0, 0, 0);
	}
			
	float3x3 ROTATION_MATRICES[6] = {
		NEG_X_ROTATION, POS_X_ROTATION, POS_Y_ROTATION,
		POS_Z_ROTATION, NEG_Z_ROTATION, NEG_Y_ROTATION
	};
	// rejig the uv to go from float2([0-1], [gutter - (1 - gutter)]) to float2([0-1], [0-1])
	float2 transformedUv = float2(uv.x, MoveValueToRange(float2(gutter, 1.0 - gutter), float2(0, 1), uv.y));
	float2 gridSize = float2(3.0, 2.0);
	float2 columnRow = GetColumnRow(transformedUv, gridSize.x, gridSize.y);			
	viewDir = GetView(transformedUv, columnRow, 1.0 / gridSize, ROTATION_MATRICES[columnRow.x + 3 * columnRow.y]);
#elif CUBE_RENDERING_MODE == FORWARD
	// recalculate the direction to show only -z
	viewDir = mul( POS_Z_ROTATION, ConvertUvToViewDir(uv) );
#elif CUBE_RENDERING_MODE == BACKWARD
	// recalculate the direction to show only z
	viewDir = mul( NEG_Z_ROTATION, ConvertUvToViewDir(uv) );
		
#elif CUBE_RENDERING_MODE == RIGHT
	// recalculate the direction to show only x
	viewDir = mul( POS_X_ROTATION, ConvertUvToViewDir(uv) );
		
#elif CUBE_RENDERING_MODE == LEFT
	// recalculate the direction to show only -x
	viewDir = mul( NEG_X_ROTATION, ConvertUvToViewDir(uv) );
		
#elif CUBE_RENDERING_MODE == UP
	// recalculate the direction to show only y
	viewDir = mul( POS_Y_ROTATION, ConvertUvToViewDir(uv) );
		
#elif CUBE_RENDERING_MODE == DOWN		
	// recalculate the direction to show only -y
	viewDir = mul( NEG_Y_ROTATION, ConvertUvToViewDir(uv) );
#else // CUBE
	viewDir = mul( ( float3x3 )ViewMat, ConvertUvToViewDir(uv) );
#endif
    return lerp( Texture.Sample( AutoMipMapTexture, viewDir ), Texture.SampleLevel( NoMipMapTexture, viewDir, ForceMipMap ), UseForceMipMap );
}
#endif


float4 BlitPS( BlitVertex inVtx ) : COLOR
{
    float2 UV = inVtx.texCoord; // * Zoom.xy + Zoom.zw;
    float4x4 ColorTransform = { ColorTransform0, ColorTransform1, ColorTransform2, ColorTransform3 };
    float4 color;

#if TEXTURE_TYPE == TEXTURE_2D
#if TEXTURE_MSAA == MSAA_YES
    uint w, h, d, s;
#if TEXTURE_ARRAY == ARRAY_NO
    Texture.GetDimensions( w, h, d );
    color = float4( Texture.Load( UV * float2( w, h ), 0 ) );
#else
	Texture.GetDimensions( w, h, d, s );
	color = float4( Texture.Load( float3(UV * float2( w, h ), ArraySlice), 0 ) );
#endif
#elif ( TEXTURE_FORMAT == FORMAT_INT || TEXTURE_FORMAT == FORMAT_UINT || TEXTURE_ARRAY == ARRAY_YES )
	uint w, h, d;
	uint mip = UseForceMipMap ? ForceMipMap : 0;
#if TEXTURE_ARRAY == ARRAY_NO
	Texture.GetDimensions( w, h );
	color = float4( Texture.Load( float3( UV * float2( w, h ), mip ) ) );
#else
	Texture.GetDimensions( w, h, d );
	color = float4( Texture.Load( float4( UV * float2( w, h ), ArraySlice, mip ) ) );
#endif
#else
	color = lerp( Texture.Sample( AutoMipMapTexture, UV ), Texture.SampleLevel( NoMipMapTexture, UV, ForceMipMap ), UseForceMipMap );
#endif
#elif TEXTURE_TYPE == TEXTURE_3D
#if ( TEXTURE_FORMAT == FORMAT_INT || TEXTURE_FORMAT == FORMAT_UINT )
	uint w, h, d;
	Texture.GetDimensions( w, h, d );
	uint mip = UseForceMipMap ? ForceMipMap : 0;
	color = float4( Texture.Load( float4( UV * float2( w, h ), VolumeSlice * d, mip ) ) );
#else
	color = lerp( Texture.Sample( AutoMipMapTexture, float3( UV, VolumeSlice ) ), Texture.SampleLevel( NoMipMapTexture, float3( UV, VolumeSlice ), ForceMipMap ), UseForceMipMap );
#endif
#else
#if ( TEXTURE_FORMAT == FORMAT_INT || TEXTURE_FORMAT == FORMAT_UINT || TEXTURE_ARRAY == ARRAY_YES )
	color = 0;
#else
	color = CubeColor(inVtx.texCoord);
#endif
#endif

#if COLOR_SPACE == SRGB_SPACE
    color = SRGBToLinear( color );
#elif COLOR_SPACE == LOGARITHMIC_SPACE
	color = LogarithmicToLinearish( color );
#endif

    color = mul( color, ColorTransform );
    color = ( color - MinValue ) / ( MaxValue - MinValue );
    color = LinearToSRGB( color );
    return float4( color.rgb, 1 );
}

technique Main
{
    pass P0
    {
        AlphaBlendEnable = False;
        
        VertexShader = compile vs_3_0 BlitVS();
        PixelShader = compile ps_3_0 BlitPS();
    }
}
