#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct DepthStencil : public WithValidRenderContext
{
};


TEST_F( DepthStencil, DepthStencilIsInvalidBeforeCreation )
{
	Tr2TextureAL ds;
	EXPECT_FALSE( ds.IsValid() );
}

TEST_F( WithRenderContext, CreatingDepthStencilWithoutRenderContextFails )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_FAILED( ds.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
	EXPECT_FALSE( ds.IsValid() );
}

TEST_F( DepthStencil, DepthStencilIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( ConvertDepthStencilFormat( DSFMT_D24S8 ), ds.GetFormat() );
	EXPECT_EQ( 1, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
}

TEST_F( DepthStencil, MsaaDepthStencilIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2MsaaDesc( 4 ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( ConvertDepthStencilFormat( DSFMT_D24S8 ), ds.GetFormat() );
	EXPECT_EQ( 4, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
}

TEST_F( DepthStencil, DepthStencilEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
	EXPECT_TRUE( ds == ds );
}

TEST_F( DepthStencil, DifferentDepthStencilsAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL ds1;
	ASSERT_HRESULT_SUCCEEDED( ds1.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
	Tr2TextureAL ds2;
	ASSERT_HRESULT_SUCCEEDED( ds2.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
	EXPECT_FALSE( ds1 == ds2 );
}

TEST_F( DepthStencil, CanCreateReadableDepthStencil )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( ConvertDepthStencilFormat( DSFMT_READABLE ), ds.GetFormat() );
	EXPECT_EQ( 1, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
	EXPECT_TRUE( Tr2GpuUsage::HasFlag( ds.GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE ) );
	EXPECT_EQ( 1, ds.GetMipCount() );
}

TEST_F( DepthStencil, DepthStencilHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );
	auto memoryClass = ds.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
