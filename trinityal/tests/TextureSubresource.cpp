// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

using namespace Tr2RenderContextEnum;

namespace
{

struct BitmapDimensionsTest : public Tr2BitmapDimensions
{
	BitmapDimensionsTest()
	{
	}
	void SetType( TextureType type )
	{
		m_type = type;
		m_arraySize = type == TEX_TYPE_CUBE ? 6 : 1;
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

TEST( TextureSubresource, TextureSubresourceDefaultConstructorCreatesFullResource )
{
	Tr2TextureSubresource ts;
	EXPECT_GE( 0u, ts.m_startFace );
	EXPECT_LE( std::numeric_limits<uint32_t>::max(), ts.m_endFace );
	EXPECT_EQ( 0, ts.m_startMipLevel );
	EXPECT_EQ( uint32_t( -1 ), ts.m_endMipLevel );
	EXPECT_FALSE( ts.HasBox() );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.left );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.top );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.front );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.right );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.bottom );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.back );
	EXPECT_TRUE( ts.IsValid() );
}

TEST( TextureSubresource, TextureSubresourceFaceAndMipLevelConstructorCreatesFullFaceMipLevel )
{
	const CubemapFace face = CUBEMAP_FACE_POSITIVE_Z;
	const uint32_t mipLevel = 2;
	Tr2TextureSubresource ts( face, mipLevel );
	EXPECT_GE( uint32_t( face ), ts.m_startFace );
	EXPECT_LE( uint32_t( face + 1 ), ts.m_endFace );
	EXPECT_EQ( mipLevel, ts.m_startMipLevel );
	EXPECT_EQ( mipLevel + 1, ts.m_endMipLevel );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.left );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.top );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.front );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.right );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.bottom );
	EXPECT_EQ( uint32_t( -1 ), ts.m_box.back );
	EXPECT_TRUE( ts.IsValid() );
}

TEST( TextureSubresource, CanClampTextureSubresourceTo2DTexture )
{
	Tr2TextureSubresource ts;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetMipCount( 3 );

	ts.ClampToTexture( dim );

	EXPECT_GE( 0u, ts.m_startFace );
	EXPECT_LE( 1u, ts.m_endFace );
	EXPECT_EQ( 0, ts.m_startMipLevel );
	EXPECT_EQ( 3, ts.m_endMipLevel );
	EXPECT_EQ( 0, ts.m_box.left );
	EXPECT_EQ( 0, ts.m_box.top );
	EXPECT_EQ( 0, ts.m_box.front );
	EXPECT_EQ( 26, ts.m_box.right );
	EXPECT_EQ( 71, ts.m_box.bottom );
	EXPECT_EQ( 1, ts.m_box.back );
}

TEST( TextureSubresource, CanClampTextureSubresourceToCubeTexture )
{
	Tr2TextureSubresource ts;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_CUBE );
	dim.SetWidth( 13 );
	dim.SetHeight( 13 );
	dim.SetMipCount( 2 );

	ts.ClampToTexture( dim );

	EXPECT_GE( 0u, ts.m_startFace );
	EXPECT_LE( 6u, ts.m_endFace );
	EXPECT_EQ( 0, ts.m_startMipLevel );
	EXPECT_EQ( 2, ts.m_endMipLevel );
	EXPECT_EQ( 0, ts.m_box.left );
	EXPECT_EQ( 0, ts.m_box.top );
	EXPECT_EQ( 0, ts.m_box.front );
	EXPECT_EQ( 13, ts.m_box.right );
	EXPECT_EQ( 13, ts.m_box.bottom );
	EXPECT_EQ( 1, ts.m_box.back );
}

TEST( TextureSubresource, CanClampTextureSubresourceTo3DTexture )
{
	Tr2TextureSubresource ts;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_3D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetDepth( 15 );
	dim.SetMipCount( 3 );

	ts.ClampToTexture( dim );

	EXPECT_GE( 0u, ts.m_startFace );
	EXPECT_LE( 1u, ts.m_endFace );
	EXPECT_EQ( 0, ts.m_startMipLevel );
	EXPECT_EQ( 3, ts.m_endMipLevel );
	EXPECT_EQ( 0, ts.m_box.left );
	EXPECT_EQ( 0, ts.m_box.top );
	EXPECT_EQ( 0, ts.m_box.front );
	EXPECT_EQ( 26, ts.m_box.right );
	EXPECT_EQ( 71, ts.m_box.bottom );
	EXPECT_EQ( 15, ts.m_box.back );
}

TEST( TextureSubresource, DefaultTextureSubresourceCovers2DTexture )
{
	Tr2TextureSubresource ts;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetMipCount( 3 );

	EXPECT_TRUE( ts.IsSubresourceFull( dim ) );
}

TEST( TextureSubresource, SmallerTextureSubresourceDoesNotCover2DTexture )
{
	Tr2TextureSubresource ts;
	ts.m_box.right = 21;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetMipCount( 3 );

	EXPECT_FALSE( ts.IsSubresourceFull( dim ) );
}

TEST( TextureSubresource, TextureSubresourceWithStartMipmapDoesNotCover2DTexture )
{
	Tr2TextureSubresource ts;
	ts.m_startMipLevel = 1;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetMipCount( 3 );

	EXPECT_FALSE( ts.IsSubresourceFull( dim ) );
}

TEST( TextureSubresource, TextureSubresourceWithSmallerMipmapsDoesNotCover2DTexture )
{
	Tr2TextureSubresource ts;
	ts.m_endMipLevel = 1;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetMipCount( 3 );

	EXPECT_FALSE( ts.IsSubresourceFull( dim ) );
}

TEST( TextureSubresource, TextureSubresourceWithOffsetDoesNotCover2DTexture )
{
	Tr2TextureSubresource ts;
	ts.m_box.top = 3;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 26 );
	dim.SetHeight( 71 );
	dim.SetMipCount( 3 );

	EXPECT_FALSE( ts.IsSubresourceFull( dim ) );
}

TEST( TextureSubresource, TextureSubresourceWithFacesDoesNotCoverCubeTexture )
{
	Tr2TextureSubresource ts;
	ts.m_startFace = CUBEMAP_FACE_NEGATIVE_Y;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_CUBE );
	dim.SetWidth( 26 );
	dim.SetHeight( 26 );

	EXPECT_FALSE( ts.IsSubresourceFull( dim ) );
}

TEST( TextureSubresource, IdenticalTextureSubresourcesAreEqual )
{
	Tr2TextureSubresource ts1;
	ts1.m_startFace = CUBEMAP_FACE_NEGATIVE_Y;
	ts1.m_box.left = 3;
	ts1.m_box.right = 5;
	ts1.m_startMipLevel = 6;

	Tr2TextureSubresource ts2 = ts1;

	EXPECT_TRUE( ts1 == ts2 );
}

TEST( TextureSubresource, DifferentTextureSubresourcesAreNotEqual )
{
	Tr2TextureSubresource ts1;
	ts1.m_startFace = CUBEMAP_FACE_NEGATIVE_Y;
	ts1.m_box.left = 3;
	ts1.m_box.right = 5;
	ts1.m_startMipLevel = 6;

	Tr2TextureSubresource ts2 = ts1;
	ts2.m_box.top = 12;

	EXPECT_FALSE( ts1 == ts2 );
}

TEST( TextureSubresource, TextureSubresourceWithIncorrectLeftAndRightIsNotValid )
{
	Tr2TextureSubresource ts;
	ts.m_box.left = 5;
	ts.m_box.right = 3;

	EXPECT_FALSE( ts.IsValid() );
}

TEST( TextureSubresource, TextureSubresourceWithIncorrectTopAndBottomIsNotValid )
{
	Tr2TextureSubresource ts;
	ts.m_box.top = 15;
	ts.m_box.bottom = 15;

	EXPECT_FALSE( ts.IsValid() );
}

TEST( TextureSubresource, TextureSubresourceWithIncorrectFrontAndBackIsNotValid )
{
	Tr2TextureSubresource ts;
	ts.m_box.front = 25;
	ts.m_box.back = 21;

	EXPECT_FALSE( ts.IsValid() );
}

TEST( TextureSubresource, TextureSubresourceWithIncorrectMipLevelsIsNotValid )
{
	Tr2TextureSubresource ts;
	ts.m_startMipLevel = 2;
	ts.m_endMipLevel = 1;

	EXPECT_FALSE( ts.IsValid() );
}

TEST( TextureSubresource, TextureSubresourceWithIncorrectFacesIsNotValid )
{
	Tr2TextureSubresource ts;
	ts.m_startFace = CubemapFace( CUBEMAP_FACE_FIRST + 1 );
	ts.m_endFace = CUBEMAP_FACE_FIRST;

	EXPECT_FALSE( ts.IsValid() );
}

TEST( TextureSubresource, TextureSubresourceReportsCorrectWidth )
{
	Tr2TextureSubresource ts;
	ts.m_box.left = 5;
	ts.m_box.right = 27;

	EXPECT_EQ( ts.m_box.right - ts.m_box.left, ts.GetWidth() );
}

TEST( TextureSubresource, TextureSubresourceReportsCorrectHeight )
{
	Tr2TextureSubresource ts;
	ts.m_box.top = 5;
	ts.m_box.bottom = 27;

	EXPECT_EQ( ts.m_box.bottom - ts.m_box.top, ts.GetHeight() );
}

TEST( TextureSubresource, TextureSubresourceReportsCorrectDepth )
{
	Tr2TextureSubresource ts;
	ts.m_box.front = 5;
	ts.m_box.back = 27;

	EXPECT_EQ( ts.m_box.back - ts.m_box.front, ts.GetDepth() );
}

TEST( TextureSubresource, TextureSubresourceReportsCorrectMipCount )
{
	Tr2TextureSubresource ts;
	ts.m_startMipLevel = 2;
	ts.m_endMipLevel = 7;

	EXPECT_EQ( ts.m_endMipLevel - ts.m_startMipLevel, ts.GetMipCount() );
}

TEST( TextureSubresource, TextureSubresourceReportsCorrectFaceCount )
{
	Tr2TextureSubresource ts;
	ts.m_startFace = CubemapFace( CUBEMAP_FACE_FIRST + 1 );
	ts.m_endFace = CubemapFace( CUBEMAP_FACE_FIRST + 3 );

	EXPECT_EQ( ts.m_endFace - ts.m_startFace, ts.GetFaceCount() );
}

TEST( TextureSubresource, CroppingTextureSubresourcesWithDifferentFaceCountFails )
{
	Tr2TextureSubresource ts1;
	ts1.m_startFace = CubemapFace( CUBEMAP_FACE_FIRST + 1 );
	ts1.m_endFace = CubemapFace( CUBEMAP_FACE_FIRST + 3 );

	BitmapDimensionsTest dim1;
	dim1.SetType( TEX_TYPE_CUBE );
	dim1.SetWidth( 26 );
	dim1.SetHeight( 26 );

	Tr2TextureSubresource ts2;
	ts2.m_startFace = CubemapFace( CUBEMAP_FACE_FIRST + 1 );
	ts2.m_endFace = CubemapFace( CUBEMAP_FACE_FIRST + 2 );

	BitmapDimensionsTest dim2;
	dim2.SetType( TEX_TYPE_CUBE );
	dim2.SetWidth( 26 );
	dim2.SetHeight( 26 );

	EXPECT_FALSE( Crop( ts1, dim1, ts2, dim2 ) );
}

TEST( TextureSubresource, CroppingFullTextureSubresourcesWithSameBitmapSucceeds )
{
	Tr2TextureSubresource ts1;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetWidth( 32 );
	dim.SetHeight( 16 );

	Tr2TextureSubresource ts2;

	EXPECT_TRUE( Crop( ts1, dim, ts2, dim ) );

	EXPECT_EQ( 0, ts1.m_startMipLevel );
	EXPECT_EQ( 6, ts1.m_endMipLevel );
	EXPECT_EQ( 0, ts1.m_box.left );
	EXPECT_EQ( 0, ts1.m_box.top );
	EXPECT_EQ( 0, ts1.m_box.front );
	EXPECT_EQ( 32, ts1.m_box.right );
	EXPECT_EQ( 16, ts1.m_box.bottom );
	EXPECT_EQ( 1, ts1.m_box.back );
	EXPECT_TRUE( ts1 == ts2 );
}

TEST( TextureSubresource, CanAdvanceMipForUncompressedTexture )
{
	Tr2TextureSubresource ts;
	ts.m_box.left = 4;
	ts.m_box.right = 24;
	ts.m_box.top = 7;
	ts.m_box.bottom = 10;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetFormat( PIXEL_FORMAT_B8G8R8A8_TYPELESS );
	dim.SetWidth( 32 );
	dim.SetHeight( 16 );

	AdvanceMip( ts, dim, 1 );

	EXPECT_EQ( 2, ts.m_box.left );
	EXPECT_EQ( 3, ts.m_box.top );
	EXPECT_EQ( 12, ts.m_box.right );
	EXPECT_EQ( 5, ts.m_box.bottom );

	AdvanceMip( ts, dim, 2 );

	EXPECT_EQ( 1, ts.m_box.left );
	EXPECT_EQ( 1, ts.m_box.top );
	EXPECT_EQ( 6, ts.m_box.right );
	EXPECT_EQ( 2, ts.m_box.bottom );
}

TEST( TextureSubresource, CanAdvanceMipForCompressedTexture )
{
	Tr2TextureSubresource ts;
	ts.m_box.left = 4;
	ts.m_box.right = 24;
	ts.m_box.top = 7;
	ts.m_box.bottom = 10;

	BitmapDimensionsTest dim;
	dim.SetType( TEX_TYPE_2D );
	dim.SetFormat( PIXEL_FORMAT_BC1_UNORM );
	dim.SetWidth( 32 );
	dim.SetHeight( 16 );

	AdvanceMip( ts, dim, 1 );

	EXPECT_EQ( 2, ts.m_box.left );
	EXPECT_EQ( 3, ts.m_box.top );
	EXPECT_EQ( 12, ts.m_box.right );
	EXPECT_EQ( 7, ts.m_box.bottom );

	AdvanceMip( ts, dim, 2 );

	EXPECT_EQ( 1, ts.m_box.left );
	EXPECT_EQ( 3, ts.m_box.top );
	EXPECT_EQ( 6, ts.m_box.right );
	EXPECT_EQ( 7, ts.m_box.bottom );
}
