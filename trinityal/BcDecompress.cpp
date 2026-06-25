// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "BcDecompress.h"

using namespace Tr2RenderContextEnum;

namespace
{
inline unsigned ConvertBGR565A8ToBGRA8( uint32_t color, uint32_t alpha )
{
	return uint32_t( color & 0x1f ) * 255 / 31 |
		( uint32_t( ( color >> 5 ) & 0x3f ) * 255 / 63 ) << 8 |
		( uint32_t( ( color >> 11 ) & 0x1f ) * 255 / 31 ) << 16 |
		( alpha << 24 );
}

inline uint32_t InterpolatedColor( uint32_t color0, uint32_t weight0, uint32_t color1, uint32_t weight1, uint32_t offset, uint32_t divisor )
{
	uint32_t r0 = ( color0 >> 16 ) & 0xff;
	uint32_t g0 = ( color0 >> 8 ) & 0xff;
	uint32_t b0 = ( color0 >> 0 ) & 0xff;
	uint32_t r1 = ( color1 >> 16 ) & 0xff;
	uint32_t g1 = ( color1 >> 8 ) & 0xff;
	uint32_t b1 = ( color1 >> 0 ) & 0xff;

	uint32_t r = ( r0 * weight0 + r1 * weight1 + offset ) / divisor & 0xff;
	uint32_t g = ( g0 * weight0 + g1 * weight1 + offset ) / divisor & 0xff;
	uint32_t b = ( b0 * weight0 + b1 * weight1 + offset ) / divisor & 0xff;
	return ( r << 16 ) | ( g << 8 ) | b;
}

void DecompressBc1( uint32_t width, uint32_t height, uint32_t depth, const Tr2SubresourceData& src, uint8_t* decompressed )
{
	uint32_t pitch = width * 4;
	uint32_t slice = pitch * height;
	for( uint32_t k = 0; k < depth; ++k )
	{
		const uint8_t* source = static_cast<const uint8_t*>( src.m_sysMem ) + src.m_sysMemSlicePitch * k;
		for( uint32_t j = 0; j < height; j += 4 )
		{
			for( uint32_t i = 0; i < width; i += 4 )
			{
				uint32_t color0 = *reinterpret_cast<const uint16_t*>( source );
				uint32_t color1 = *reinterpret_cast<const uint16_t*>( source + 2 );
				uint32_t bits = *reinterpret_cast<const uint32_t*>( source + 4 );
				if( color0 > color1 )
				{
					color0 = ConvertBGR565A8ToBGRA8( color0, 0 );
					color1 = ConvertBGR565A8ToBGRA8( color1, 0 );
					uint32_t color2 = InterpolatedColor( color0, 2, color1, 1, 1, 3 );
					uint32_t color3 = InterpolatedColor( color0, 1, color1, 2, 1, 3 );
					for( uint32_t y = 0; y < 4; ++y )
					{
						for( uint32_t x = 0; x < 4; ++x )
						{
							uint32_t destY = j + y;
							uint32_t* destPixel = reinterpret_cast<uint32_t*>( decompressed + k * slice + destY * pitch + ( x + i ) * sizeof( uint32_t ) );
							switch( ( bits >> 2 * ( 4 * y + x ) ) & 3 )
							{
							case 0:
								*destPixel = color0 | 0xff000000;
								break;
							case 1:
								*destPixel = color1 | 0xff000000;
								break;
							case 2:
								*destPixel = color2 | 0xff000000;
								break;
							case 3:
								*destPixel = color3 | 0xff000000;
								break;
							}
						}
					}
				}
				else
				{
					color0 = ConvertBGR565A8ToBGRA8( color0, 0 );
					color1 = ConvertBGR565A8ToBGRA8( color1, 0 );
					uint32_t color2 = InterpolatedColor( color0, 1, color1, 1, 0, 2 );
					for( unsigned y = 0; y < 4; ++y )
					{
						for( unsigned x = 0; x < 4; ++x )
						{
							uint32_t destY = j + y;
							uint32_t* destPixel = reinterpret_cast<uint32_t*>( decompressed + k * slice + destY * pitch + ( x + i ) * sizeof( uint32_t ) );
							switch( ( bits >> 2 * ( 4 * y + x ) ) & 3 )
							{
							case 0:
								*destPixel = color0 | 0xff000000;
								break;
							case 1:
								*destPixel = color1 | 0xff000000;
								break;
							case 2:
								*destPixel = color2 | 0xff000000;
								break;
							case 3:
								*destPixel = color2;
								break;
							}
						}
					}
				}
				source += 8;
			}
		}
	}
}

void DecompressBc2( uint32_t width, uint32_t height, uint32_t depth, const Tr2SubresourceData& src, uint8_t* decompressed )
{
	uint32_t pitch = width * 4;
	uint32_t slice = pitch * height;
	for( uint32_t k = 0; k < depth; ++k )
	{
		const uint8_t* source = static_cast<const uint8_t*>( src.m_sysMem ) + src.m_sysMemSlicePitch * k;
		for( uint32_t j = 0; j < height; j += 4 )
		{
			for( uint32_t i = 0; i < width; i += 4 )
			{
				uint32_t color0 = ConvertBGR565A8ToBGRA8( *reinterpret_cast<const uint16_t*>( source + 8 ), 0 );
				uint32_t color1 = ConvertBGR565A8ToBGRA8( *reinterpret_cast<const uint16_t*>( source + 8 + 2 ), 0 );
				uint32_t color2 = InterpolatedColor( color0, 2, color1, 1, 1, 3 );
				uint32_t color3 = InterpolatedColor( color0, 1, color1, 2, 1, 3 );
				uint32_t bits = *reinterpret_cast<const uint32_t*>( source + 8 + 4 );
				for( uint32_t y = 0; y < 4; ++y )
				{
					for( uint32_t x = 0; x < 4; ++x )
					{
						uint32_t destY = j + y;
						uint32_t* destPixel = reinterpret_cast<uint32_t*>( decompressed + k * slice + destY * pitch + ( x + i ) * sizeof( uint32_t ) );
						unsigned alphaValue = ( *( reinterpret_cast<const uint16_t*>( source ) + y ) >> x * 4 ) & 15;
						alphaValue = alphaValue * 255 / 15;
						alphaValue <<= 24;
						switch( ( bits >> 2 * ( 4 * y + x ) ) & 3 )
						{
						case 0:
							*destPixel = color0 | alphaValue;
							break;
						case 1:
							*destPixel = color1 | alphaValue;
							break;
						case 2:
							*destPixel = color2 | alphaValue;
							break;
						case 3:
							*destPixel = color3 | alphaValue;
							break;
						}
					}
				}
				source += 16;
			}
		}
	}
}

void DecompressBc3( uint32_t width, uint32_t height, uint32_t depth, const Tr2SubresourceData& src, uint8_t* decompressed )
{
	uint32_t pitch = width * 4;
	uint32_t slice = pitch * height;
	for( uint32_t k = 0; k < depth; ++k )
	{
		const uint8_t* source = static_cast<const uint8_t*>( src.m_sysMem ) + src.m_sysMemSlicePitch * k;
		for( uint32_t j = 0; j < height; j += 4 )
		{
			for( uint32_t i = 0; i < width; i += 4 )
			{
				uint32_t alpha[8];
				alpha[0] = *reinterpret_cast<const uint8_t*>( source );
				alpha[1] = *reinterpret_cast<const uint8_t*>( source + 1 );
				auto alphaMask = source + 2;
				uint32_t alphaMask0 = ( alphaMask[0] ) | ( alphaMask[1] << 8 ) | ( alphaMask[2] << 16 );
				uint32_t alphaMask1 = ( alphaMask[3] ) | ( alphaMask[4] << 8 ) | ( alphaMask[5] << 16 );
				if( alpha[0] > alpha[1] )
				{
					alpha[2] = ( 6 * alpha[0] + 1 * alpha[1] + 3 ) / 7;
					alpha[3] = ( 5 * alpha[0] + 2 * alpha[1] + 3 ) / 7;
					alpha[4] = ( 4 * alpha[0] + 3 * alpha[1] + 3 ) / 7;
					alpha[5] = ( 3 * alpha[0] + 4 * alpha[1] + 3 ) / 7;
					alpha[6] = ( 2 * alpha[0] + 5 * alpha[1] + 3 ) / 7;
					alpha[7] = ( 1 * alpha[0] + 6 * alpha[1] + 3 ) / 7;
				}
				else
				{
					alpha[2] = ( 4 * alpha[0] + 1 * alpha[1] + 2 ) / 5;
					alpha[3] = ( 3 * alpha[0] + 2 * alpha[1] + 2 ) / 5;
					alpha[4] = ( 2 * alpha[0] + 3 * alpha[1] + 2 ) / 5;
					alpha[5] = ( 1 * alpha[0] + 4 * alpha[1] + 2 ) / 5;
					alpha[6] = 0;
					alpha[7] = 255;
				}

				uint32_t color0 = ConvertBGR565A8ToBGRA8( *reinterpret_cast<const uint16_t*>( source + 8 ), 0 );
				uint32_t color1 = ConvertBGR565A8ToBGRA8( *reinterpret_cast<const uint16_t*>( source + 8 + 2 ), 0 );
				uint32_t color2 = InterpolatedColor( color0, 2, color1, 1, 1, 3 );
				uint32_t color3 = InterpolatedColor( color0, 1, color1, 2, 1, 3 );
				uint32_t bits = *reinterpret_cast<const uint32_t*>( source + 8 + 4 );
				for( uint32_t y = 0; y < 4; ++y )
				{
					for( uint32_t x = 0; x < 4; ++x )
					{
						uint32_t alphaValue;
						if( y < 2 )
						{
							alphaValue = alpha[( alphaMask0 >> ( x + y * 4 ) * 3 ) & 0x7];
						}
						else
						{
							alphaValue = alpha[( alphaMask1 >> ( x + ( y - 2 ) * 4 ) * 3 ) & 0x7];
						}
						alphaValue <<= 24;
						uint32_t* destPixel = reinterpret_cast<uint32_t*>( decompressed + k * slice + ( y + j ) * pitch + ( x + i ) * sizeof( uint32_t ) );
						switch( ( bits >> 2 * ( 4 * y + x ) ) & 3 )
						{
						case 0:
							*destPixel = color0 | alphaValue;
							break;
						case 1:
							*destPixel = color1 | alphaValue;
							break;
						case 2:
							*destPixel = color2 | alphaValue;
							break;
						case 3:
							*destPixel = color3 | alphaValue;
							break;
						}
					}
				}
				source += 16;
			}
		}
	}
}

}

bool BcDecompress( uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, const Tr2SubresourceData& src, std::unique_ptr<uint8_t[]>& decompressed )
{
	if( !decompressed )
	{
		decompressed.reset( new uint8_t[width * height * depth * 4] );
	}
	switch( format )
	{
	case PIXEL_FORMAT_BC1_TYPELESS:
	case PIXEL_FORMAT_BC1_UNORM:
	case PIXEL_FORMAT_BC1_UNORM_SRGB:
		DecompressBc1( width, height, depth, src, decompressed.get() );
		return true;

	case PIXEL_FORMAT_BC2_TYPELESS:
	case PIXEL_FORMAT_BC2_UNORM:
	case PIXEL_FORMAT_BC2_UNORM_SRGB:
		DecompressBc2( width, height, depth, src, decompressed.get() );
		return true;

	case PIXEL_FORMAT_BC3_TYPELESS:
	case PIXEL_FORMAT_BC3_UNORM:
	case PIXEL_FORMAT_BC3_UNORM_SRGB:
		DecompressBc3( width, height, depth, src, decompressed.get() );
		return true;

	default:
		return false;
	}
}
