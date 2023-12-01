#include "StdAfx.h"

using namespace Tr2RenderContextEnum;

namespace
{

struct BitmapDimensionsTest: public Tr2BitmapDimensions
{
	BitmapDimensionsTest() {}
	void SetType( TextureType type )
	{
		m_type = type;
	}
	void SetFormat( PixelFormat format )
	{
		m_format = format;
	}
	void SetWidth( uint32_t width )
	{
		m_width = width;
	}
	void SetHeight( uint32_t height )
	{
		m_height = height;
	}
	void SetDepth( uint32_t depth )
	{
		m_volumeDepth = depth;
	}
	void SetMipCount( uint32_t mipCount )
	{
		m_mipCount = mipCount;
	}
};

}

TEST( BitmapDimensions, InitialSizeIsZero )
{
	Tr2BitmapDimensions dim;
	EXPECT_EQ( 0, dim.GetWidth() );
	EXPECT_EQ( 0, dim.GetHeight() );
	EXPECT_EQ( 0, dim.GetDepth() );
	EXPECT_EQ( 0, dim.GetTrueMipCount() );
	EXPECT_EQ( 0, dim.GetMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_UNKNOWN, dim.GetFormat() );
	EXPECT_EQ( TEX_TYPE_INVALID, dim.GetType() );
}

TEST( BitmapDimensions, ReportsCorrect2DSizes )
{
	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 32 );
	dim.SetHeight( 16 );
	dim.SetMipCount( 2 );


	EXPECT_EQ( TEX_TYPE_2D, dim.GetType() );
	EXPECT_EQ( 32, dim.GetWidth() );
	EXPECT_EQ( 16, dim.GetHeight() );
	EXPECT_EQ( 0, dim.GetDepth() );
	EXPECT_EQ( 2, dim.GetTrueMipCount() );
	EXPECT_EQ( 2, dim.GetMipCount() );
}

TEST( BitmapDimensions, ReportsCorrect3DSizes )
{
	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_3D );
	dim.SetWidth( 32 );
	dim.SetHeight( 16 );
	dim.SetDepth( 64 );
	dim.SetMipCount( 2 );


	EXPECT_EQ( TEX_TYPE_3D, dim.GetType() );
	EXPECT_EQ( 32, dim.GetWidth() );
	EXPECT_EQ( 16, dim.GetHeight() );
	EXPECT_EQ( 64, dim.GetDepth() );
	EXPECT_EQ( 2, dim.GetTrueMipCount() );
	EXPECT_EQ( 2, dim.GetMipCount() );
}

TEST( BitmapDimensions, CalculatesTrueMipCountAndMipSizes )
{
	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_3D );

	uint32_t width = 32;
	uint32_t height = 16;
	uint32_t depth = 64;

	dim.SetWidth( width );
	dim.SetHeight( height );
	dim.SetDepth( depth );
	dim.SetMipCount( 0 );

	uint32_t level = 0;
	while( depth > 1  )
	{
		EXPECT_EQ( width, dim.GetMipWidth( level ) );
		EXPECT_EQ( height, dim.GetMipHeight( level ) );
		EXPECT_EQ( depth, dim.GetMipDepth( level ) );
		width = std::max( width / 2, 1u );
		height = std::max( height / 2, 1u );
		depth /= 2;
		++level;
	}
	EXPECT_EQ( level, dim.GetTrueMipCount() );
}

TEST( BitmapDimensions, GettingMipSizesForIncorrectMipReturnsZero )
{
	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_3D );

	uint32_t width = 32;
	uint32_t height = 16;
	uint32_t depth = 64;

	dim.SetWidth( width );
	dim.SetHeight( height );
	dim.SetDepth( depth );
	dim.SetMipCount( 0 );

	EXPECT_EQ( 0, dim.GetMipWidth( 10 ) );
	EXPECT_EQ( 0, dim.GetMipHeight( 10 ) );
	EXPECT_EQ( 0, dim.GetMipDepth( 10 ) );
}

TEST( BitmapDimensions, CalculatesTrueMipCountAndMipSizesForNptTexture )
{
	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );

	uint32_t width = 731;
	uint32_t height = 317;

	dim.SetWidth( width );
	dim.SetHeight( height );
	dim.SetMipCount( 0 );

	uint32_t level = 0;
	while( width > 0 )
	{
		EXPECT_EQ( width, dim.GetMipWidth( level ) );
		EXPECT_EQ( height, dim.GetMipHeight( level ) );
		width /= 2;
		height = std::max( height / 2, 1u );
		++level;
	}
	EXPECT_EQ( level, dim.GetTrueMipCount() );
}

TEST( BitmapDimensions, CalculatesCorrectMipSize )
{
	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );

	uint32_t width = 32;
	uint32_t height = 16;

	dim.SetWidth( width );
	dim.SetHeight( height );
	dim.SetMipCount( 0 );

	Tr2RenderContextEnum::PixelFormat formats[] = {
		Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM,
		Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT,
		Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM, };
	uint32_t bpp[] = {
		32,
		64,
		4, };
	for( uint32_t i = 0; i < sizeof( formats ) / sizeof( formats[0] ); ++i )
	{
		dim.SetFormat( formats[i] );
		EXPECT_EQ( width * height * bpp[i] / 8, dim.GetMipSize( 0 ) );
		EXPECT_EQ( width / 2 * height / 2 * bpp[i] / 8, dim.GetMipSize( 1 ) );
	}
}
