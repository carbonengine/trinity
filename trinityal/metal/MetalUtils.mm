// Copyright © 2020 CCP ehf.

#if TRINITY_PLATFORM == TRINITY_METAL
#import <Foundation/Foundation.h>
#include "MetalUtils.h"
#include "ALLog.h"


namespace TrinityALImpl
{

void MetalUtils::SetupPixelFormatConversionTable()
{
	// Safe guard future expansion of formats
	for( int i = 0; i < Tr2RenderContextEnum::PIXEL_FORMAT_SENTINEL; i++ )
	{
		PixelFormatConversionTable[i] = MTLPixelFormatInvalid;
	}

	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT] = MTLPixelFormatRGBA32Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT] = MTLPixelFormatRGBA32Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_SINT] = MTLPixelFormatRGBA32Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_UINT] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_SINT] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT] = MTLPixelFormatRGBA16Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM] = MTLPixelFormatRGBA16Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UINT] = MTLPixelFormatRGBA16Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_SNORM] = MTLPixelFormatRGBA16Snorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_SINT] = MTLPixelFormatRGBA16Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_FLOAT] = MTLPixelFormatRG32Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_UINT] = MTLPixelFormatRG32Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_SINT] = MTLPixelFormatRG32Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32G8X24_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT_S8X24_UINT] =
		MTLPixelFormatDepth32Float_Stencil8;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_UNORM] = MTLPixelFormatRGB10A2Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_UINT] = MTLPixelFormatRGB10A2Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R11G11B10_FLOAT] = MTLPixelFormatRG11B10Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM] = MTLPixelFormatRGBA8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB] = MTLPixelFormatRGBA8Unorm_sRGB;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UINT] = MTLPixelFormatRGBA8Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SNORM] = MTLPixelFormatRGBA8Snorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SINT] = MTLPixelFormatRGBA8Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT] = MTLPixelFormatRG16Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_UNORM] = MTLPixelFormatRG16Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_UINT] = MTLPixelFormatRG16Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_SNORM] = MTLPixelFormatRG16Snorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_SINT] = MTLPixelFormatRG16Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT] = MTLPixelFormatDepth32Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT] = MTLPixelFormatR32Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT] = MTLPixelFormatR32Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT] = MTLPixelFormatR32Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R24G8_TYPELESS] = MTLPixelFormatInvalid;
	// HACK: D24S8 is emulated on Intel/AMD and is not supported on Apple Silicon. Use D32 or D32S8 instead.
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT] = MTLPixelFormatDepth32Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R24_UNORM_X8_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_X24_TYPELESS_G8_UINT] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM] = MTLPixelFormatRG8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UINT] = MTLPixelFormatRG8Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_SNORM] = MTLPixelFormatRG8Snorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_SINT] = MTLPixelFormatRG8Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16_FLOAT] = MTLPixelFormatR16Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_D16_UNORM] = MTLPixelFormatDepth16Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16_UNORM] = MTLPixelFormatR16Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT] = MTLPixelFormatR16Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16_SNORM] = MTLPixelFormatR16Snorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R16_SINT] = MTLPixelFormatR16Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM] = MTLPixelFormatR8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8_UINT] = MTLPixelFormatR8Uint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8_SNORM] = MTLPixelFormatR8Snorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8_SINT] = MTLPixelFormatR8Sint;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_A8_UNORM] = MTLPixelFormatA8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R1_UNORM] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R9G9B9E5_SHAREDEXP] = MTLPixelFormatRGB9E5Float;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_B8G8_UNORM] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_G8R8_G8B8_UNORM] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC1_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM] = MTLPixelFormatBC1_RGBA;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM_SRGB] = MTLPixelFormatBC1_RGBA_sRGB;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC2_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM] = MTLPixelFormatBC2_RGBA;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM_SRGB] = MTLPixelFormatBC2_RGBA_sRGB;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC3_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM] = MTLPixelFormatBC3_RGBA;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM_SRGB] = MTLPixelFormatBC3_RGBA_sRGB;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC4_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC4_UNORM] = MTLPixelFormatBC4_RUnorm; // Check
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC4_SNORM] = MTLPixelFormatBC4_RSnorm; // Check
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC5_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC5_UNORM] = MTLPixelFormatBC5_RGUnorm; // Check
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC5_SNORM] = MTLPixelFormatBC5_RGSnorm; // Check
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B5G6R5_UNORM] = MTLPixelFormatInvalid; // iOS only
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B5G5R5A1_UNORM] = MTLPixelFormatInvalid; // iOS only
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM] = MTLPixelFormatBGRA8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM] = MTLPixelFormatBGRA8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_TYPELESS] = MTLPixelFormatBGRA8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB] = MTLPixelFormatBGRA8Unorm_sRGB;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_TYPELESS] = MTLPixelFormatBGRA8Unorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB] = MTLPixelFormatBGRA8Unorm_sRGB;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC6H_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC6H_UF16] = MTLPixelFormatBC6H_RGBUfloat;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC6H_SF16] = MTLPixelFormatBC6H_RGBFloat;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC7_TYPELESS] = MTLPixelFormatInvalid;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC7_UNORM] = MTLPixelFormatBC7_RGBAUnorm;
	PixelFormatConversionTable[Tr2RenderContextEnum::PIXEL_FORMAT_BC7_UNORM_SRGB] = MTLPixelFormatBC7_RGBAUnorm_sRGB;
}

MTLPixelFormat MetalUtils::GetMTLPixelFormat( Tr2RenderContextEnum::PixelFormat pixelFormat )
{
	if( pixelFormat < Tr2RenderContextEnum::PIXEL_FORMAT_SENTINEL )
	{
		return PixelFormatConversionTable[pixelFormat];
	}
	else
	{
		return MTLPixelFormatInvalid;
	}
}

MTLTextureType
	MetalUtils::GetMTLTextureType( Tr2RenderContextEnum::TextureType type, uint32 arrayLength, uint32 sampleCount )
{
	switch( type )
	{
	case Tr2RenderContextEnum::TEX_TYPE_1D:
		return ( arrayLength > 1 ) ? MTLTextureType1DArray : MTLTextureType1D;
	case Tr2RenderContextEnum::TEX_TYPE_2D:
		if( sampleCount > 1 )
		{
			return ( arrayLength > 1 ) ? MTLTextureType2DMultisampleArray : MTLTextureType2DMultisample;
		}
		else
		{
			return ( arrayLength > 1 ) ? MTLTextureType2DArray : MTLTextureType2D;
		}
	case Tr2RenderContextEnum::TEX_TYPE_3D:
		return MTLTextureType3D;
	case Tr2RenderContextEnum::TEX_TYPE_CUBE:
		return MTLTextureTypeCube;
		break;
	default:
		CCP_AL_LOGERR( "Unknown texture format: %d. Defaulting to MTLTextureType2D.", (int)type );
		return MTLTextureType2D;
	}
}

// May have to use a switch statement here if the enum values grow large since they appear to be comprised of bitwise ops.
void MetalUtils::SetupVertexFormatConversionTable()
{
	// Because the enums aren't a series of increasing values the table will be sparse so initialise it
	for( int i = 0; i < METAL_SIZEOF_VERTEX_FORMAT_TABLE; i++ )
	{
		VertexFormatConversionTable[i] = MTLVertexFormatInvalid;
	}

	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_1] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_2] = MTLVertexFormatChar2;
	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_3] = MTLVertexFormatChar3;
	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_4] = MTLVertexFormatChar4;

	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_1] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_2] = MTLVertexFormatUChar2;
	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_3] = MTLVertexFormatUChar3;
	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_4] = MTLVertexFormatUChar4;

	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_1_NORM] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_2_NORM] = MTLVertexFormatChar2Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_3_NORM] = MTLVertexFormatChar3Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::BYTE_4_NORM] = MTLVertexFormatChar4Normalized;

	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_1_NORM] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_2_NORM] = MTLVertexFormatUChar2Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_3_NORM] = MTLVertexFormatUChar3Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::UBYTE_4_NORM] = MTLVertexFormatUChar4Normalized;


	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_1] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_2] = MTLVertexFormatShort2;
	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_3] = MTLVertexFormatShort3;
	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_4] = MTLVertexFormatShort4;

	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_1] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_2] = MTLVertexFormatUShort2;
	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_3] = MTLVertexFormatUShort3;
	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_4] = MTLVertexFormatUShort4;

	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_1_NORM] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_2_NORM] = MTLVertexFormatShort2Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_3_NORM] = MTLVertexFormatShort3Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::SHORT_4_NORM] = MTLVertexFormatShort4Normalized;

	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_1_NORM] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_2_NORM] = MTLVertexFormatUShort2Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_3_NORM] = MTLVertexFormatUShort3Normalized;
	VertexFormatConversionTable[Tr2VertexDefinition::USHORT_4_NORM] = MTLVertexFormatUShort4Normalized;

	VertexFormatConversionTable[Tr2VertexDefinition::INT32_1] = MTLVertexFormatInt;
	VertexFormatConversionTable[Tr2VertexDefinition::INT32_2] = MTLVertexFormatInt2;
	VertexFormatConversionTable[Tr2VertexDefinition::INT32_3] = MTLVertexFormatInt3;
	VertexFormatConversionTable[Tr2VertexDefinition::INT32_4] = MTLVertexFormatInt4;

	VertexFormatConversionTable[Tr2VertexDefinition::UINT32_1] = MTLVertexFormatUInt;
	VertexFormatConversionTable[Tr2VertexDefinition::UINT32_2] = MTLVertexFormatUInt2;
	VertexFormatConversionTable[Tr2VertexDefinition::UINT32_3] = MTLVertexFormatUInt3;
	VertexFormatConversionTable[Tr2VertexDefinition::UINT32_4] = MTLVertexFormatUInt4;

	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT16_1] = MTLVertexFormatInvalid;
	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT16_2] = MTLVertexFormatHalf2;
	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT16_3] = MTLVertexFormatHalf3;
	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT16_4] = MTLVertexFormatHalf4;

	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT32_1] = MTLVertexFormatFloat;
	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT32_2] = MTLVertexFormatFloat2;
	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT32_3] = MTLVertexFormatFloat3;
	VertexFormatConversionTable[Tr2VertexDefinition::FLOAT32_4] = MTLVertexFormatFloat4;
}

MTLVertexFormat MetalUtils::GetMTLVertexFormat( Tr2VertexDefinition::DataType dataType )
{
	return VertexFormatConversionTable[dataType];
}

MTLCullMode MetalUtils::GetMTLCullMode( Tr2RenderContextEnum::CullMode cullMode )
{
	switch( cullMode )
	{
	case Tr2RenderContextEnum::CULLMODE_NONE:
		return MTLCullModeNone;
	case Tr2RenderContextEnum::CULLMODE_CW:
		return MTLCullModeFront;
	case Tr2RenderContextEnum::CULLMODE_CCW:
		return MTLCullModeBack;
	}
}

MTLBlendFactor MetalUtils::GetMTLBlendFactor( uint32_t value )
{
	// It seems like Trinity uses the D3D enums directly here
	static const MTLBlendFactor BlendFormatConversionTable[] = {
		(MTLBlendFactor)-1, // --
		MTLBlendFactorZero, // D3DBLEND_ZERO
		MTLBlendFactorOne, // D3DBLEND_ONE
		MTLBlendFactorSourceColor, // D3DBLEND_SRCCOLOR
		MTLBlendFactorOneMinusSourceColor, // D3DBLEND_INVSRCCOLOR
		MTLBlendFactorSourceAlpha, // D3DBLEND_SRCALPHA
		MTLBlendFactorOneMinusSourceAlpha, // D3DBLEND_INVSRCALPHA
		MTLBlendFactorDestinationAlpha, // D3DBLEND_DESTALPHA
		MTLBlendFactorOneMinusDestinationAlpha, // D3DBLEND_INVDESTALPHA
		MTLBlendFactorDestinationColor, // D3DBLEND_DESTCOLOR
		MTLBlendFactorOneMinusDestinationColor, // D3DBLEND_INVDESTCOLOR
		MTLBlendFactorSourceAlphaSaturated, // D3DBLEND_SRCALPHASAT
		MTLBlendFactorSource1Alpha, // D3DBLEND_BOTHSRCALPHA
		MTLBlendFactorOneMinusSource1Alpha, // D3DBLEND_BOTHINVSRCALPHA
		MTLBlendFactorBlendColor, // D3DBLEND_BLENDFACTOR
		MTLBlendFactorOneMinusBlendColor, // D3DBLEND_INVBLENDFACTOR
		MTLBlendFactorSource1Color, // D3DBLEND_SRCCOLOR2
		MTLBlendFactorOneMinusSource1Color, // D3DBLEND_INVSRCCOLOR2

	};

	return BlendFormatConversionTable[value];
}

MTLCompareFunction MetalUtils::GetMTLCompareFunction( uint32_t value )
{
	static const MTLCompareFunction CompareFunctionCoversionTable[] = {
		(MTLCompareFunction)-1, // --
		MTLCompareFunctionNever, // D3DCMP_NEVER
		MTLCompareFunctionLess, // D3DCMP_LESS
		MTLCompareFunctionEqual, // D3DCMP_EQUAL
		MTLCompareFunctionLessEqual, // D3DCMP_LESSEQUAL
		MTLCompareFunctionGreater, // D3DCMP_GREATER
		MTLCompareFunctionNotEqual, // D3DCMP_NOTEQUAL
		MTLCompareFunctionGreaterEqual, // D3DCMP_GREATEREQUAL
		MTLCompareFunctionAlways, // D3DCMP_ALWAYS
	};

	return CompareFunctionCoversionTable[value];
}

MTLStencilOperation MetalUtils::GetMTLStencilOperation( uint32_t value )
{
	static const MTLStencilOperation StencilOperationConversionTable[] = {
		(MTLStencilOperation)-1, // ---
		MTLStencilOperationKeep, // D3DSTENCILOP_KEEP
		MTLStencilOperationZero, // D3DSTENCILOP_ZERO
		MTLStencilOperationReplace, // D3DSTENCILOP_REPLACE
		MTLStencilOperationIncrementClamp, // D3DSTENCILOP_INCRSAT
		MTLStencilOperationDecrementClamp, // D3DSTENCILOP_DECRSAT
		MTLStencilOperationInvert, // D3DSTENCILOP_INVERT
		MTLStencilOperationIncrementWrap, // D3DSTENCILOP_INCR
		MTLStencilOperationDecrementWrap, // D3DSTENCILOP_DECR
	};

	return StencilOperationConversionTable[value];
}

MTLBlendOperation MetalUtils::GetMTLBlendOperation( uint32_t value )
{
	static const MTLBlendOperation BlendOperationConversionTable[] = {
		(MTLBlendOperation)-1, // ---
		MTLBlendOperationAdd, // D3DBLENDOP_ADD
		MTLBlendOperationSubtract, // D3DBLENDOP_SUBTRACT
		MTLBlendOperationReverseSubtract, // D3DBLENDOP_REVSUBTRACT
		MTLBlendOperationMin, // D3DBLENDOP_MIN
		MTLBlendOperationMax, // D3DBLENDOP_MAX
	};

	return BlendOperationConversionTable[value];
}

MTLColorWriteMask MetalUtils::GetMTLColorWriteMask( Tr2RenderContextEnum::ColorWriteEnable writeMask )
{
	MTLColorWriteMask metalWriteMask = MTLColorWriteMaskNone;
	if( writeMask & Tr2RenderContextEnum::COLORWRITEENABLE_RED )
		metalWriteMask |= MTLColorWriteMaskRed;
	if( writeMask & Tr2RenderContextEnum::COLORWRITEENABLE_GREEN )
		metalWriteMask |= MTLColorWriteMaskGreen;
	if( writeMask & Tr2RenderContextEnum::COLORWRITEENABLE_BLUE )
		metalWriteMask |= MTLColorWriteMaskBlue;
	if( writeMask & Tr2RenderContextEnum::COLORWRITEENABLE_ALPHA )
		metalWriteMask |= MTLColorWriteMaskAlpha;
	return metalWriteMask;
}

MetalColor MetalUtils::GetMetalColorFromRGBA8( uint32_t color )
{
	MetalColor metalColor;
	metalColor.red = float( ( color >> 0 ) & 0xff ) / 255.f;
	metalColor.green = float( ( color >> 8 ) & 0xff ) / 255.f;
	metalColor.blue = float( ( color >> 16 ) & 0xff ) / 255.f;
	metalColor.alpha = float( ( color >> 24 ) & 0xff ) / 255.f;

	return metalColor;
}

MetalUtils::MetalUtils()
{
	SetupPixelFormatConversionTable();
	SetupVertexFormatConversionTable();
}

MetalUtils::~MetalUtils()
{
}


ConstantBufferAllocator::ConstantBufferAllocator() : m_device( nullptr ), m_offset( 0 ), m_totalUploadedSize( 0 )
{
	std::fill( std::begin( m_pages ), std::end( m_pages ), nullptr );
}

void ConstantBufferAllocator::Initialize( id<MTLDevice> device )
{
	m_device = device;
	std::fill( std::begin( m_pages ), std::end( m_pages ), nullptr );
	std::fill( std::begin( m_pageContents ), std::end( m_pageContents ), nullptr );
	CreatePage( 0 );
	m_offset = 0;
	m_totalUploadedSize = 0;
}

void ConstantBufferAllocator::Reset()
{
	m_offset = 0;
	m_totalUploadedSize = 0;
}

uint32_t ConstantBufferAllocator::GetTotalUploadedSize() const
{
	return m_totalUploadedSize + ( m_offset & 0xffffffff );
}

ConstantBufferAllocator::Entry ConstantBufferAllocator::Allocate( const void* data, uint32_t size )
{
	auto dataSize = size;
	size = ( size + ( CONST_ALIGNMENT - 1 ) ) & ~( CONST_ALIGNMENT - 1 );

	uint64_t offset = m_offset.fetch_add( size, std::memory_order_relaxed );
	if( ( offset & 0xffffffff ) + size > CONST_PAGE_SIZE )
	{
		m_pagesMutex.lock();
		offset = m_offset.fetch_add( size, std::memory_order_relaxed );
		if( ( offset & 0xffffffff ) + size > CONST_PAGE_SIZE )
		{
			auto pageIndex = offset >> 32;
			++pageIndex;
			if( !m_pages[pageIndex] )
			{
				CreatePage( uint32_t( pageIndex ) );
			}
			m_totalUploadedSize += m_offset & 0xffffffff;
			m_offset = ( pageIndex << 32 ) | size;
			offset = pageIndex << 32;
		}
		m_pagesMutex.unlock();
	}
	Entry entry;
	entry.offset = offset & 0xffffffff;
	entry.page = offset >> 32;
	if( data )
	{
		memcpy( m_pageContents[entry.page] + entry.offset, data, dataSize );
	}
	return entry;
}

id<MTLBuffer> ConstantBufferAllocator::GetPage( uint32_t page )
{
	return m_pages[page];
}

void ConstantBufferAllocator::CreatePage( uint32_t index )
{
	auto options = MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;
	if( @available( macOS 10.15, * ) )
	{
		options |= MTLHazardTrackingModeUntracked;
	}
	m_pages[index] = [m_device newBufferWithLength:CONST_PAGE_SIZE options:options];
	m_pages[index].label = [NSString stringWithUTF8String:"Constant buffer"];
	m_pageContents[index] = static_cast<uint8_t*>( m_pages[index].contents );
}


} // TrinityALImpl

#endif
