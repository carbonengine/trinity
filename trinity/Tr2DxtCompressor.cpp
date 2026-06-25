// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2DxtCompressor.h"
#include <squish.h>

typedef unsigned short word;
typedef unsigned int dword;

#define INSET_COLOR_SHIFT 4 // inset color bounding box
#define INSET_ALPHA_SHIFT 5 // inset alpha bounding box

#define C565_5_MASK 0xF8 // 0xFF minus last three bits
#define C565_6_MASK 0xFC // 0xFF minus last two bits

#define NVIDIA_G7X_HARDWARE_BUG_FIX // keep the colors sorted as: max, min

/* 
RGB <-> YCoCg

Y  = [ 1/4  1/2  1/4] [R]
Co = [ 1/2    0 -1/2] [G]
CG = [-1/4  1/2 -1/4] [B]

R  = [   1    1   -1] [Y]
G  = [   1    0    1] [Co]
B  = [   1   -1   -1] [Cg]

*/

#define RGB_TO_YCOCG_Y( r, g, b ) ( ( ( r + ( g << 1 ) + b ) + 2 ) >> 2 )
#define RGB_TO_YCOCG_CO( r, g, b ) ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 )
#define RGB_TO_YCOCG_CG( r, g, b ) ( ( ( -r + ( g << 1 ) - b ) + 2 ) >> 2 )

#define COCG_TO_R( co, cg ) ( co - cg )
#define COCG_TO_G( co, cg ) ( cg )
#define COCG_TO_B( co, cg ) ( -co - cg )

uint8_t CLAMP_BYTE( int x )
{
	return ( ( x ) < 0 ? ( 0 ) : ( ( x ) > 255 ? 255 : ( x ) ) );
}

void ConvertRGBToCoCg_Y( uint8_t* image, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int r = image[i * 4 + 0];
		int g = image[i * 4 + 1];
		int b = image[i * 4 + 2];
		int a = image[i * 4 + 3];
		image[i * 4 + 0] = CLAMP_BYTE( RGB_TO_YCOCG_CO( r, g, b ) + 128 );
		image[i * 4 + 1] = CLAMP_BYTE( RGB_TO_YCOCG_CG( r, g, b ) + 128 );
		image[i * 4 + 2] = a;
		image[i * 4 + 3] = CLAMP_BYTE( RGB_TO_YCOCG_Y( r, g, b ) );
	}
}

void ConvertCoCg_YToRGB( uint8_t* image, int width, int height )
{
	for( int i = 0; i < width * height; i++ )
	{
		int y = image[i * 4 + 3];
		int co = image[i * 4 + 0] - 128;
		int cg = image[i * 4 + 1] - 128;
		int a = image[i * 4 + 2];
		image[i * 4 + 0] = CLAMP_BYTE( y + COCG_TO_R( co, cg ) );
		image[i * 4 + 1] = CLAMP_BYTE( y + COCG_TO_G( co, cg ) );
		image[i * 4 + 2] = CLAMP_BYTE( y + COCG_TO_B( co, cg ) );
		image[i * 4 + 3] = a;
	}
}

void GetMinMaxColors( const uint8_t* colorBlock, uint8_t* minColor, uint8_t* maxColor )
{
	int i;
	uint8_t inset[3];
	minColor[0] = minColor[1] = minColor[2] = 255;
	maxColor[0] = maxColor[1] = maxColor[2] = 0;
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < minColor[0] )
		{
			minColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < minColor[1] )
		{
			minColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < minColor[2] )
		{
			minColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 0] > maxColor[0] )
		{
			maxColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > maxColor[1] )
		{
			maxColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > maxColor[2] )
		{
			maxColor[2] = colorBlock[i * 4 + 2];
		}
	}
	inset[0] = ( maxColor[0] - minColor[0] ) >> INSET_COLOR_SHIFT;
	inset[1] = ( maxColor[1] - minColor[1] ) >> INSET_COLOR_SHIFT;
	inset[2] = ( maxColor[2] - minColor[2] ) >> INSET_COLOR_SHIFT;
	minColor[0] = ( minColor[0] + inset[0] <= 255 ) ? minColor[0] + inset[0] : 255;
	minColor[1] = ( minColor[1] + inset[1] <= 255 ) ? minColor[1] + inset[1] : 255;
	minColor[2] = ( minColor[2] + inset[2] <= 255 ) ? minColor[2] + inset[2] : 255;
	maxColor[0] = ( maxColor[0] >= inset[0] ) ? maxColor[0] - inset[0] : 0;
	maxColor[1] = ( maxColor[1] >= inset[1] ) ? maxColor[1] - inset[1] : 0;
	maxColor[2] = ( maxColor[2] >= inset[2] ) ? maxColor[2] - inset[2] : 0;
}

void GetMinMaxColorsWithAlpha( const uint8_t* colorBlock, uint8_t* minColor, uint8_t* maxColor )
{
	int i;
	uint8_t inset[4];
	minColor[0] = minColor[1] = minColor[2] = minColor[3] = 255;
	maxColor[0] = maxColor[1] = maxColor[2] = maxColor[3] = 0;
	for( i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < minColor[0] )
		{
			minColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < minColor[1] )
		{
			minColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < minColor[2] )
		{
			minColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] < minColor[3] )
		{
			minColor[3] = colorBlock[i * 4 + 3];
		}
		if( colorBlock[i * 4 + 0] > maxColor[0] )
		{
			maxColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > maxColor[1] )
		{
			maxColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > maxColor[2] )
		{
			maxColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] > maxColor[3] )
		{
			maxColor[3] = colorBlock[i * 4 + 3];
		}
	}
	inset[0] = ( maxColor[0] - minColor[0] ) >> INSET_COLOR_SHIFT;
	inset[1] = ( maxColor[1] - minColor[1] ) >> INSET_COLOR_SHIFT;
	inset[2] = ( maxColor[2] - minColor[2] ) >> INSET_COLOR_SHIFT;
	inset[3] = ( maxColor[3] - minColor[3] ) >> INSET_COLOR_SHIFT;
	minColor[0] = ( minColor[0] + inset[0] <= 255 ) ? minColor[0] + inset[0] : 255;
	minColor[1] = ( minColor[1] + inset[1] <= 255 ) ? minColor[1] + inset[1] : 255;
	minColor[2] = ( minColor[2] + inset[2] <= 255 ) ? minColor[2] + inset[2] : 255;
	minColor[3] = ( minColor[3] + inset[3] <= 255 ) ? minColor[3] + inset[3] : 255;
	maxColor[0] = ( maxColor[0] >= inset[0] ) ? maxColor[0] - inset[0] : 0;
	maxColor[1] = ( maxColor[1] >= inset[1] ) ? maxColor[1] - inset[1] : 0;
	maxColor[2] = ( maxColor[2] >= inset[2] ) ? maxColor[2] - inset[2] : 0;
	maxColor[3] = ( maxColor[3] >= inset[3] ) ? maxColor[3] - inset[3] : 0;
}

word ColorTo565( const uint8_t* color )
{
	// BGR
	return ( ( color[2] >> 3 ) << 11 ) | ( ( color[1] >> 2 ) << 5 ) | ( color[0] >> 3 );
}

word NormalYTo565( uint8_t y )
{
	return ( ( y >> 2 ) << 5 );
}

void EmitByte( uint8_t b, uint8_t** outBuff )
{
	**outBuff = b;
	*outBuff += 1;
}

void EmitWord( word s, uint8_t** outBuff )
{
	**outBuff = ( s >> 0 ) & 255;
	*( *outBuff + 1 ) = ( s >> 8 ) & 255;
	*outBuff += 2;
}

void EmitDoubleWord( dword i, uint8_t** outBuff )
{
	**outBuff = ( i >> 0 ) & 255;
	*( *outBuff + 1 ) = ( i >> 8 ) & 255;
	*( *outBuff + 2 ) = ( i >> 16 ) & 255;
	*( *outBuff + 3 ) = ( i >> 24 ) & 255;
	*outBuff += 4;
}

void ExtractBlock( const uint8_t* inPtr, const int width, uint8_t* colorBlock )
{
	for( int j = 0; j < 4; j++ )
	{
		memcpy( &colorBlock[j * 4 * 4], inPtr, 4 * 4 );
		inPtr += width * 4;
	}
}

// Extract a block, but blank out blue, and swap red and alpha, on the fly.
void ExtractBlockDXT5N( const uint8_t* inPtr, const int width, uint8_t* colorBlock )
{
	for( int j = 0; j != 4; ++j )
	{
		const uint8_t* src = inPtr;
		for( int i = 0; i != 4; ++i, src += 4 )
		{
			*colorBlock++ = 0;
			*colorBlock++ = src[1];
			*colorBlock++ = src[3];
			*colorBlock++ = src[2];
		}

		inPtr += width * 4;
	}
}

void GetMinMaxYCoCg( uint8_t* colorBlock, uint8_t* minColor, uint8_t* maxColor )
{
	minColor[0] = minColor[1] = minColor[2] = minColor[3] = 255;
	maxColor[0] = maxColor[1] = maxColor[2] = maxColor[3] = 0;

	for( int i = 0; i < 16; i++ )
	{
		if( colorBlock[i * 4 + 0] < minColor[0] )
		{
			minColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] < minColor[1] )
		{
			minColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] < minColor[2] )
		{
			minColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] < minColor[3] )
		{
			minColor[3] = colorBlock[i * 4 + 3];
		}
		if( colorBlock[i * 4 + 0] > maxColor[0] )
		{
			maxColor[0] = colorBlock[i * 4 + 0];
		}
		if( colorBlock[i * 4 + 1] > maxColor[1] )
		{
			maxColor[1] = colorBlock[i * 4 + 1];
		}
		if( colorBlock[i * 4 + 2] > maxColor[2] )
		{
			maxColor[2] = colorBlock[i * 4 + 2];
		}
		if( colorBlock[i * 4 + 3] > maxColor[3] )
		{
			maxColor[3] = colorBlock[i * 4 + 3];
		}
	}
}

void ScaleYCoCg( uint8_t* colorBlock, uint8_t* minColor, uint8_t* maxColor )
{
	int m0 = abs( minColor[0] - 128 );
	int m1 = abs( minColor[1] - 128 );
	int m2 = abs( maxColor[0] - 128 );
	int m3 = abs( maxColor[1] - 128 );

	if( m1 > m0 )
		m0 = m1;
	if( m3 > m2 )
		m2 = m3;
	if( m2 > m0 )
		m0 = m2;

	const int s0 = 128 / 2 - 1;
	const int s1 = 128 / 4 - 1;

	int mask0 = -( m0 <= s0 );
	int mask1 = -( m0 <= s1 );
	int scale = 1 + ( 1 & mask0 ) + ( 2 & mask1 );

	minColor[0] = ( minColor[0] - 128 ) * scale + 128;
	minColor[1] = ( minColor[1] - 128 ) * scale + 128;
	minColor[2] = ( scale - 1 ) << 3;

	maxColor[0] = ( maxColor[0] - 128 ) * scale + 128;
	maxColor[1] = ( maxColor[1] - 128 ) * scale + 128;
	maxColor[2] = ( scale - 1 ) << 3;

	for( int i = 0; i < 16; i++ )
	{
		colorBlock[i * 4 + 0] = ( colorBlock[i * 4 + 0] - 128 ) * scale + 128;
		colorBlock[i * 4 + 1] = ( colorBlock[i * 4 + 1] - 128 ) * scale + 128;
	}
}

void InsetYCoCgBBox( uint8_t* minColor, uint8_t* maxColor )
{
	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxColor[0] - minColor[0] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxColor[1] - minColor[1] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );
	inset[3] = ( maxColor[3] - minColor[3] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );

	mini[0] = ( ( minColor[0] << INSET_COLOR_SHIFT ) + inset[0] ) >> INSET_COLOR_SHIFT;
	mini[1] = ( ( minColor[1] << INSET_COLOR_SHIFT ) + inset[1] ) >> INSET_COLOR_SHIFT;
	mini[3] = ( ( minColor[3] << INSET_ALPHA_SHIFT ) + inset[3] ) >> INSET_ALPHA_SHIFT;

	maxi[0] = ( ( maxColor[0] << INSET_COLOR_SHIFT ) - inset[0] ) >> INSET_COLOR_SHIFT;
	maxi[1] = ( ( maxColor[1] << INSET_COLOR_SHIFT ) - inset[1] ) >> INSET_COLOR_SHIFT;
	maxi[3] = ( ( maxColor[3] << INSET_ALPHA_SHIFT ) - inset[3] ) >> INSET_ALPHA_SHIFT;

	mini[0] = ( mini[0] >= 0 ) ? mini[0] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;
	mini[3] = ( mini[3] >= 0 ) ? mini[3] : 0;

	maxi[0] = ( maxi[0] <= 255 ) ? maxi[0] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;
	maxi[3] = ( maxi[3] <= 255 ) ? maxi[3] : 255;

	minColor[0] = ( mini[0] & C565_5_MASK ) | ( mini[0] >> 5 );
	minColor[1] = ( mini[1] & C565_6_MASK ) | ( mini[1] >> 6 );
	minColor[3] = mini[3];

	maxColor[0] = ( maxi[0] & C565_5_MASK ) | ( maxi[0] >> 5 );
	maxColor[1] = ( maxi[1] & C565_6_MASK ) | ( maxi[1] >> 6 );
	maxColor[3] = maxi[3];
}

void SelectYCoCgDiagonal( const uint8_t* colorBlock, uint8_t* minColor, uint8_t* maxColor )
{
	uint8_t mid0 = ( (int)minColor[0] + maxColor[0] + 1 ) >> 1;
	uint8_t mid1 = ( (int)minColor[1] + maxColor[1] + 1 ) >> 1;

	uint8_t side = 0;
	for( int i = 0; i < 16; i++ )
	{
		uint8_t b0 = colorBlock[i * 4 + 0] >= mid0;
		uint8_t b1 = colorBlock[i * 4 + 1] >= mid1;
		side += ( b0 ^ b1 );
	}

	uint8_t mask = -( side > 8 );

#ifdef NVIDIA_7X_HARDWARE_BUG_FIX
	mask &= -( minColor[0] != maxColor[0] );
#endif

	uint8_t c0 = minColor[1];
	uint8_t c1 = maxColor[1];

	c0 ^= c1;
	mask &= c0;
	c1 ^= mask;
	c0 ^= c1;

	minColor[1] = c0;
	maxColor[1] = c1;
}

void EmitAlphaIndices( const uint8_t* colorBlock, const uint8_t minAlpha, const uint8_t maxAlpha, uint8_t** outBuff )
{

	CCP_ASSERT( maxAlpha >= minAlpha );

	const int ALPHA_RANGE = 7;

	uint8_t mid, ab1, ab2, ab3, ab4, ab5, ab6, ab7;
	uint8_t indexes[16];

	mid = ( maxAlpha - minAlpha ) / ( 2 * ALPHA_RANGE );

	ab1 = minAlpha + mid;
	ab2 = ( 6 * maxAlpha + 1 * minAlpha ) / ALPHA_RANGE + mid;
	ab3 = ( 5 * maxAlpha + 2 * minAlpha ) / ALPHA_RANGE + mid;
	ab4 = ( 4 * maxAlpha + 3 * minAlpha ) / ALPHA_RANGE + mid;
	ab5 = ( 3 * maxAlpha + 4 * minAlpha ) / ALPHA_RANGE + mid;
	ab6 = ( 2 * maxAlpha + 5 * minAlpha ) / ALPHA_RANGE + mid;
	ab7 = ( 1 * maxAlpha + 6 * minAlpha ) / ALPHA_RANGE + mid;

	for( int i = 0; i < 16; i++ )
	{
		uint8_t a = colorBlock[i * 4 + 3];
		int b1 = ( a <= ab1 );
		int b2 = ( a <= ab2 );
		int b3 = ( a <= ab3 );
		int b4 = ( a <= ab4 );
		int b5 = ( a <= ab5 );
		int b6 = ( a <= ab6 );
		int b7 = ( a <= ab7 );
		int index = ( b1 + b2 + b3 + b4 + b5 + b6 + b7 + 1 ) & 7;
		indexes[i] = index ^ ( 2 > index );
	}

	EmitByte( ( indexes[0] >> 0 ) | ( indexes[1] << 3 ) | ( indexes[2] << 6 ), outBuff );
	EmitByte( ( indexes[2] >> 2 ) | ( indexes[3] << 1 ) | ( indexes[4] << 4 ) | ( indexes[5] << 7 ), outBuff );
	EmitByte( ( indexes[5] >> 1 ) | ( indexes[6] << 2 ) | ( indexes[7] << 5 ), outBuff );

	EmitByte( ( indexes[8] >> 0 ) | ( indexes[9] << 3 ) | ( indexes[10] << 6 ), outBuff );
	EmitByte( ( indexes[10] >> 2 ) | ( indexes[11] << 1 ) | ( indexes[12] << 4 ) | ( indexes[13] << 7 ), outBuff );
	EmitByte( ( indexes[13] >> 1 ) | ( indexes[14] << 2 ) | ( indexes[15] << 5 ), outBuff );
}

void EmitColorIndices( const uint8_t* colorBlock, const uint8_t* minColor, const uint8_t* maxColor, uint8_t** outBuff )
{
	word colors[4][4];
	dword result = 0;
	colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
	colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );
	colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 5 );
	colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
	colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );
	colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 5 );
	colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
	colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
	colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
	colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
	colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
	colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;
	for( int i = 15; i >= 0; i-- )
	{
		int c0 = colorBlock[i * 4 + 0];
		int c1 = colorBlock[i * 4 + 1];
		int c2 = colorBlock[i * 4 + 2];
		int d0 = abs( colors[0][0] - c0 ) + abs( colors[0][1] - c1 ) + abs( colors[0][2] - c2 );
		int d1 = abs( colors[1][0] - c0 ) + abs( colors[1][1] - c1 ) + abs( colors[1][2] - c2 );
		int d2 = abs( colors[2][0] - c0 ) + abs( colors[2][1] - c1 ) + abs( colors[2][2] - c2 );
		int d3 = abs( colors[3][0] - c0 ) + abs( colors[3][1] - c1 ) + abs( colors[3][2] - c2 );
		int b0 = d0 > d3;
		int b1 = d1 > d2;
		int b2 = d0 > d2;
		int b3 = d1 > d3;
		int b4 = d2 > d3;
		int x0 = b1 & b2;
		int x1 = b0 & b3;
		int x2 = b0 & b4;
		result |= ( x2 | ( ( x0 | x1 ) << 1 ) ) << ( i << 1 );
	}
	EmitDoubleWord( result, outBuff );
}
void GetMinMaxNormalsBBox( const uint8_t* block, uint8_t* minNormal, uint8_t* maxNormal )
{

	minNormal[0] = minNormal[1] = 255;
	maxNormal[0] = maxNormal[1] = 0;

	for( int i = 0; i < 16; i++ )
	{
		if( block[i * 4 + 2] < minNormal[0] )
		{
			minNormal[0] = block[i * 4 + 2];
		}
		if( block[i * 4 + 1] < minNormal[1] )
		{
			minNormal[1] = block[i * 4 + 1];
		}
		if( block[i * 4 + 2] > maxNormal[0] )
		{
			maxNormal[0] = block[i * 4 + 2];
		}
		if( block[i * 4 + 1] > maxNormal[1] )
		{
			maxNormal[1] = block[i * 4 + 1];
		}
	}
}

void InsetNormalsBBoxDXT5( uint8_t* minNormal, uint8_t* maxNormal )
{
	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxNormal[0] - minNormal[0] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxNormal[1] - minNormal[1] ) - ( ( 1 << ( INSET_COLOR_SHIFT - 1 ) ) - 1 );

	mini[0] = ( ( minNormal[0] << INSET_ALPHA_SHIFT ) + inset[0] ) >> INSET_ALPHA_SHIFT;
	mini[1] = ( ( minNormal[1] << INSET_COLOR_SHIFT ) + inset[1] ) >> INSET_COLOR_SHIFT;

	maxi[0] = ( ( maxNormal[0] << INSET_ALPHA_SHIFT ) - inset[0] ) >> INSET_ALPHA_SHIFT;
	maxi[1] = ( ( maxNormal[1] << INSET_COLOR_SHIFT ) - inset[1] ) >> INSET_COLOR_SHIFT;

	mini[0] = ( mini[0] >= 0 ) ? mini[0] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;

	maxi[0] = ( maxi[0] <= 255 ) ? maxi[0] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;

	minNormal[0] = mini[0];
	minNormal[1] = ( mini[1] & C565_6_MASK ) | ( mini[1] >> 6 );

	maxNormal[0] = maxi[0];
	maxNormal[1] = ( maxi[1] & C565_6_MASK ) | ( maxi[1] >> 6 );
}

void InsetNormalsBBox3Dc( uint8_t* minNormal, uint8_t* maxNormal )
{
	int inset[4];
	int mini[4];
	int maxi[4];

	inset[0] = ( maxNormal[0] - minNormal[0] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );
	inset[1] = ( maxNormal[1] - minNormal[1] ) - ( ( 1 << ( INSET_ALPHA_SHIFT - 1 ) ) - 1 );

	mini[0] = ( ( minNormal[0] << INSET_ALPHA_SHIFT ) + inset[0] ) >> INSET_ALPHA_SHIFT;
	mini[1] = ( ( minNormal[1] << INSET_ALPHA_SHIFT ) + inset[1] ) >> INSET_ALPHA_SHIFT;

	maxi[0] = ( ( maxNormal[0] << INSET_ALPHA_SHIFT ) - inset[0] ) >> INSET_ALPHA_SHIFT;
	maxi[1] = ( ( maxNormal[1] << INSET_ALPHA_SHIFT ) - inset[1] ) >> INSET_ALPHA_SHIFT;

	mini[0] = ( mini[0] >= 0 ) ? mini[0] : 0;
	mini[1] = ( mini[1] >= 0 ) ? mini[1] : 0;

	maxi[0] = ( maxi[0] <= 255 ) ? maxi[0] : 255;
	maxi[1] = ( maxi[1] <= 255 ) ? maxi[1] : 255;

	minNormal[0] = mini[0];
	minNormal[1] = mini[1];

	maxNormal[0] = maxi[0];
	maxNormal[1] = maxi[1];
}

void EmitAlphaIndicesForNormals( const uint8_t* block, const int offset, const uint8_t minAlpha, const uint8_t maxAlpha, uint8_t** outBuff )
{
	uint8_t mid = ( maxAlpha - minAlpha ) / ( 2 * 7 );

	uint8_t ab1 = maxAlpha - mid;
	uint8_t ab2 = ( 6 * maxAlpha + 1 * minAlpha ) / 7 - mid;
	uint8_t ab3 = ( 5 * maxAlpha + 2 * minAlpha ) / 7 - mid;
	uint8_t ab4 = ( 4 * maxAlpha + 3 * minAlpha ) / 7 - mid;
	uint8_t ab5 = ( 3 * maxAlpha + 4 * minAlpha ) / 7 - mid;
	uint8_t ab6 = ( 2 * maxAlpha + 5 * minAlpha ) / 7 - mid;
	uint8_t ab7 = ( 1 * maxAlpha + 6 * minAlpha ) / 7 - mid;

	block += offset;

	uint8_t indices[16];
	for( int i = 0; i < 16; i++ )
	{
		uint8_t a = block[i * 4];
		int b1 = ( a >= ab1 );
		int b2 = ( a >= ab2 );
		int b3 = ( a >= ab3 );
		int b4 = ( a >= ab4 );
		int b5 = ( a >= ab5 );
		int b6 = ( a >= ab6 );
		int b7 = ( a >= ab7 );
		int index = ( 8 - b1 - b2 - b3 - b4 - b5 - b6 - b7 ) & 7;
		indices[i] = index ^ ( 2 > index );
	}

	EmitByte( ( indices[0] >> 0 ) | ( indices[1] << 3 ) | ( indices[2] << 6 ), outBuff );
	EmitByte( ( indices[2] >> 2 ) | ( indices[3] << 1 ) | ( indices[4] << 4 ) | ( indices[5] << 7 ), outBuff );
	EmitByte( ( indices[5] >> 1 ) | ( indices[6] << 2 ) | ( indices[7] << 5 ), outBuff );

	EmitByte( ( indices[8] >> 0 ) | ( indices[9] << 3 ) | ( indices[10] << 6 ), outBuff );
	EmitByte( ( indices[10] >> 2 ) | ( indices[11] << 1 ) | ( indices[12] << 4 ) | ( indices[13] << 7 ), outBuff );
	EmitByte( ( indices[13] >> 1 ) | ( indices[14] << 2 ) | ( indices[15] << 5 ), outBuff );
}

void EmitGreenIndices( const uint8_t* block, const int offset, const uint8_t minGreen, const uint8_t maxGreen, uint8_t** outBuff )
{
	uint8_t mid = ( maxGreen - minGreen ) / ( 2 * 3 );

	uint8_t gb1 = maxGreen - mid;
	uint8_t gb2 = ( 2 * maxGreen + 1 * minGreen ) / 3 - mid;
	uint8_t gb3 = ( 1 * maxGreen + 2 * minGreen ) / 3 - mid;

	block += offset;

	unsigned int result = 0;
	for( int i = 15; i >= 0; i-- )
	{
		result <<= 2;
		uint8_t g = block[i * 4];
		int b1 = ( g >= gb1 );
		int b2 = ( g >= gb2 );
		int b3 = ( g >= gb3 );
		int index = ( 4 - b1 - b2 - b3 ) & 3;
		index ^= ( 2 > index );
		result |= index;
	}

	EmitDoubleWord( result, outBuff );
}

void CompressYCoCgDXT5( const uint8_t* inBuf, uint8_t* outBuf, int width, int height, ptrdiff_t& outputBytes, volatile const bool& cancel )
{
	uint8_t block[64];
	uint8_t minColor[4];
	uint8_t maxColor[4];

	uint8_t** outData = &outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		if( cancel )
		{
			return;
		}
		for( int i = 0; i < width; i += 4 )
		{

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxYCoCg( block, minColor, maxColor );
			ScaleYCoCg( block, minColor, maxColor );
			InsetYCoCgBBox( minColor, maxColor );
			SelectYCoCgDiagonal( block, minColor, maxColor );

			EmitByte( maxColor[3], outData );
			EmitByte( minColor[3], outData );

			EmitAlphaIndices( block, minColor[3], maxColor[3], outData );

			EmitWord( ColorTo565( maxColor ), outData );
			EmitWord( ColorTo565( minColor ), outData );

			EmitColorIndices( block, minColor, maxColor, outData );
		}
	}

	outputBytes = *outData - outBuf;
}

void CompressImageSquish( const uint8_t* inBuf,
						  unsigned width,
						  unsigned height,
						  uint8_t* outBuf,
						  size_t outputPitch,
						  int squish_flags,
						  volatile const bool& cancel )
{
	uint8_t block[64];
	const unsigned outDx = ( squish_flags & squish::kDxt1 ) ? 8 : 16;
	for( unsigned j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		uint8_t* outData = outBuf + ( j / 4 ) * outputPitch;

		if( cancel )
		{
			return;
		}

		for( unsigned i = 0; i < width; i += 4 )
		{
			ExtractBlock( inBuf + i * 4, width, block );
			// Swap the R and B
			// Squish wants RGB but our format is BGR
			for( int k = 0; k < 16; ++k )
			{
				std::swap( block[k * 4], block[k * 4 + 2] );
			}
			squish::Compress( block, outData, squish_flags );
			outData += outDx;
		}
	}
}

void CompressImageDXT5( const uint8_t* inBuf, uint8_t* outBuf, int width, int height, ptrdiff_t& outputBytes, volatile const bool& cancel )
{
	uint8_t block[64];
	uint8_t minColor[4];
	uint8_t maxColor[4];
	uint8_t** outData = &outBuf;
	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		if( cancel )
		{
			return;
		}
		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlock( inBuf + i * 4, width, block );
			GetMinMaxColorsWithAlpha( block, minColor, maxColor );
			EmitByte( maxColor[3], outData );
			EmitByte( minColor[3], outData );
			EmitAlphaIndices( block, minColor[3], maxColor[3], outData );
			EmitWord( ColorTo565( maxColor ), outData );
			EmitWord( ColorTo565( minColor ), outData );
			EmitColorIndices( block, minColor, maxColor, outData );
		}
	}
	outputBytes = *outData - outBuf;
}

void CompressNormalMapDXT5( const uint8_t* inBuf, uint8_t* outBuf, int width, int height, ptrdiff_t& outputBytes, volatile const bool& cancel )
{
	uint8_t block[64];
	uint8_t minColor[4];
	uint8_t maxColor[4];
	uint8_t** outData = &outBuf;
	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		if( cancel )
		{
			return;
		}
		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlockDXT5N( inBuf + i * 4, width, block );
			GetMinMaxColorsWithAlpha( block, minColor, maxColor );
			EmitByte( maxColor[3], outData );
			EmitByte( minColor[3], outData );
			EmitAlphaIndices( block, minColor[3], maxColor[3], outData );
			EmitWord( ColorTo565( maxColor ), outData );
			EmitWord( ColorTo565( minColor ), outData );
			EmitColorIndices( block, minColor, maxColor, outData );
		}
	}
	outputBytes = *outData - outBuf;
}

void CompressImageDXT1( const uint8_t* inBuf, uint8_t* outBuf, int width, int height, ptrdiff_t& outputBytes, volatile const bool& cancel )
{
	uint8_t block[64];
	uint8_t minColor[4];
	uint8_t maxColor[4];
	uint8_t** outData = &outBuf;
	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		if( cancel )
		{
			return;
		}
		for( int i = 0; i < width; i += 4 )
		{
			ExtractBlock( inBuf + i * 4, width, block );
			GetMinMaxColors( block, minColor, maxColor );
			EmitWord( ColorTo565( maxColor ), outData );
			EmitWord( ColorTo565( minColor ), outData );
			EmitColorIndices( block, minColor, maxColor, outData );
		}
	}
	outputBytes = *outData - outBuf;
}

// below works fine, but doesn't support a material id in the alpha channel
void CompressNormalMapDXT5NoAlpha( const uint8_t* inBuf, uint8_t* outBuf, int width, int height, ptrdiff_t& outputBytes, volatile const bool& cancel )
{
	uint8_t block[64];
	uint8_t normalMin[4];
	uint8_t normalMax[4];

	uint8_t** outData = &outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		if( cancel )
		{
			return;
		}
		for( int i = 0; i < width; i += 4 )
		{

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxNormalsBBox( block, normalMin, normalMax );
			InsetNormalsBBoxDXT5( normalMin, normalMax );

			// Write out Nx into alpha channel.
			// Note: all code in this file assumes TRIFMT_A8R8G8B8, so red is at [2]
			EmitByte( normalMax[0], outData );
			EmitByte( normalMin[0], outData );
			EmitAlphaIndicesForNormals( block, 2, normalMin[0], normalMax[0], outData );

			// Write out Ny into green channel.
			EmitWord( NormalYTo565( normalMax[1] ), outData );
			EmitWord( NormalYTo565( normalMin[1] ), outData );
			EmitGreenIndices( block, 1, normalMin[1], normalMax[1], outData );
		}
	}

	outputBytes = *outData - outBuf;
}

void CompressNormalMap3Dc( const uint8_t* inBuf, uint8_t* outBuf, int width, int height, ptrdiff_t& outputBytes, volatile const bool& cancel )
{
	uint8_t block[64];
	uint8_t normalMin[4];
	uint8_t normalMax[4];

	uint8_t** outData = &outBuf;

	for( int j = 0; j < height; j += 4, inBuf += width * 4 * 4 )
	{
		if( cancel )
		{
			return;
		}
		for( int i = 0; i < width; i += 4 )
		{

			ExtractBlock( inBuf + i * 4, width, block );

			GetMinMaxNormalsBBox( block, normalMin, normalMax );
			InsetNormalsBBox3Dc( normalMin, normalMax );

			// Write out Nx as an alpha channel.
			EmitByte( normalMax[0], outData );
			EmitByte( normalMin[0], outData );
			EmitAlphaIndicesForNormals( block, 0, normalMin[0], normalMax[0], outData );

			// Write out Ny as an alpha channel.
			EmitByte( normalMax[1], outData );
			EmitByte( normalMin[1], outData );
			EmitAlphaIndicesForNormals( block, 1, normalMin[1], normalMax[1], outData );
		}
	}

	outputBytes = *outData - outBuf;
}

bool Tr2DxtCompressSurface( Tr2DxtCompressionFormat eCompressFmt,
							const uint8_t* inBuf,
							unsigned width,
							unsigned height,
							uint8_t* outBuf,
							size_t outputPitch,
							volatile const bool& cancel,
							Tr2DxtCompressionSquishQuality squishQualityEnum )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	int squishQuality = squish::kColourRangeFit;

	ptrdiff_t outputBytes = 0;

	switch( squishQualityEnum )
	{
	case TR2DXT_COMPRESS_SQ_ITER_CLUSTER_FIT:
		squishQuality = squish::kColourIterativeClusterFit;
		break;

	case TR2DXT_COMPRESS_SQ_CLUSTER_FIT:
		squishQuality = squish::kColourClusterFit;
		break;

	case TR2DXT_COMPRESS_SQ_RANGE_FIT:
		squishQuality = squish::kColourRangeFit;
		break;
	default:
		break;
	}

	switch( eCompressFmt )
	{
	case TR2DXT_COMPRESS_RT_DXT1:
		CompressImageDXT1( (const uint8_t*)inBuf, (uint8_t*)outBuf, width, height, outputBytes, cancel );
		break;

	case TR2DXT_COMPRESS_RT_DXT5:
		CompressImageDXT5( (const uint8_t*)inBuf, (uint8_t*)outBuf, width, height, outputBytes, cancel );
		break;

	case TR2DXT_COMPRESS_RT_DXT5N:
		CompressNormalMapDXT5( (const uint8_t*)inBuf, (uint8_t*)outBuf, width, height, outputBytes, cancel );
		break;

	case TR2DXT_COMPRESS_RT_YCOCGDXT5:
		ConvertRGBToCoCg_Y( (uint8_t*)inBuf, width, height );
		CompressYCoCgDXT5( (const uint8_t*)inBuf, (uint8_t*)outBuf, width, height, outputBytes, cancel );
		break;

	case TR2DXT_COMPRESS_RT_3DC:
		CompressNormalMap3Dc( (const uint8_t*)inBuf, (uint8_t*)outBuf, width, height, outputBytes, cancel );
		break;

	case TR2DXT_COMPRESS_SQUISH_DXT1:
		CompressImageSquish( (const uint8_t*)inBuf,
							 width,
							 height,
							 (uint8_t*)outBuf,
							 outputPitch,
							 squish::kDxt1 | squishQuality,
							 cancel );
		return true;

	case TR2DXT_COMPRESS_SQUISH_DXT3:
		CompressImageSquish( (const uint8_t*)inBuf,
							 width,
							 height,
							 (uint8_t*)outBuf,
							 outputPitch,
							 squish::kDxt3 | squishQuality,
							 cancel );
		return true;

	case TR2DXT_COMPRESS_SQUISH_DXT5:
		CompressImageSquish( (const uint8_t*)inBuf,
							 width,
							 height,
							 (uint8_t*)outBuf,
							 outputPitch,
							 squish::kDxt5 | squishQuality,
							 cancel );
		return true;

	default:
		return false;
	}
	return true;
}

BLUE_DECLARE( BlueCallbackMan );
IBlueCallbackManPtr s_wodBackgroundCompressor;

Tr2DxtCompressControl::Tr2DxtCompressControl() :
	m_cancel( false ), m_isDone( false ), m_id( -1 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Cancels texture compression if it is not done yet.
// --------------------------------------------------------------------------------------
void Tr2DxtCompressControl::Cancel()
{
	m_cancel = true;
	if( s_wodBackgroundCompressor && !m_isDone )
	{
		s_wodBackgroundCompressor->Cancel( m_id );
		m_isDone = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if the compression is finished.
// Return Value:
//   true if compression is finished
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2DxtCompressControl::IsDone() const
{
	return m_isDone;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if the compression is requested to be canceled.
// Return Value:
//   true if compression is requested to be canceled
//   false otherwise
// --------------------------------------------------------------------------------------
const bool& Tr2DxtCompressControl::IsCanceling() const
{
	return m_cancel;
}

// --------------------------------------------------------------------------------------
// Description:
//   Notify the control that the compression is finished. Should only be called from
//   compression task itself.
// --------------------------------------------------------------------------------------
void Tr2DxtCompressControl::Done()
{
	m_isDone = true;
}

struct Tr2DxtCompressSurfaceAsyncData
{
	Tr2DxtCompressControl* control;
	bool succeeded;
	Tr2DxtCompressionFormat eCompressFmt;

	const uint8_t* inBuf;
	unsigned width;
	unsigned height;

	uint8_t* outBuf;

	size_t outputPitch;
	int squishQuality;
};

static void Tr2DxtCompressSurfaceAsyncHelper( void* context )
{
	Tr2DxtCompressSurfaceAsyncData* data = static_cast<Tr2DxtCompressSurfaceAsyncData*>( context );
	data->succeeded = Tr2DxtCompressSurface( data->eCompressFmt,
											 data->inBuf,
											 data->width,
											 data->height,
											 data->outBuf,
											 data->outputPitch,
											 data->control->IsCanceling(),
											 static_cast<Tr2DxtCompressionSquishQuality>( data->squishQuality ) );
	data->control->Done();
}

bool Tr2DxtCompressSurfaceAsync( Tr2DxtCompressionFormat eCompressFmt,
								 const unsigned char* inBuf,
								 unsigned width,
								 unsigned height,
								 unsigned char* outBuf,
								 size_t outputPitch,
								 Tr2DxtCompressControl* control,
								 int squishQuality )
{
#if BLUE_WITH_PYTHON
	if( !PyOS->CanYield() )
	{
		//this is a tasklet that cannot block
		PyErr_SetString( PyExc_RuntimeError, "Tr2DxtCompressSurfaceAsync: This tasklet cannot block" );
		return false;
	}
#endif

	if( !s_wodBackgroundCompressor )
	{
		// Create the background welder the first time we need to build
		s_wodBackgroundCompressor = BeCallbackMan;
	}

	Tr2DxtCompressSurfaceAsyncData* data = CCP_NEW( "Tr2DxtCompressSurfaceAsync/data" ) Tr2DxtCompressSurfaceAsyncData;
	data->control = control;
	data->eCompressFmt = eCompressFmt;
	data->inBuf = inBuf;
	data->outBuf = outBuf;
	data->width = width;
	data->height = height;
	data->outputPitch = outputPitch;
	data->squishQuality = squishQuality;

	s_wodBackgroundCompressor->Add( Tr2DxtCompressSurfaceAsyncHelper, (void*)data, 0, &data->control->m_id );

	while( !data->control->IsDone() )
	{
#if BLUE_WITH_PYTHON
		if( !PyOS->Yield() )
		{
			// This is 'normal', it happens when yielding a tasklet that has already been
			// killed.  Shouldn't be converting this into a runtime error, dixit Kristjan.
			// Ideally we'd want to catch the exception here and check it really is a PyExc_TaskletExit,
			// however by the time Yield returns it's already logged through the global _ExceptionHandler
			// in uicore.py
			data->control->Cancel();
			s_wodBackgroundCompressor->Cancel( data->control->m_id );
			break;
		}
#endif
	}

	const bool result = data->succeeded;

	CCP_DELETE data;

	return result;
}
