// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct RenderTarget : public WithValidRenderContext
{
};


TEST_F( RenderTarget, RenderTargetIsInvalidBeforeCreation )
{
	Tr2TextureAL rt;
	EXPECT_FALSE( rt.IsValid() );
}

TEST_F( WithRenderContext, CreatingRenderTargetWithoutRenderContextFails )
{
	Tr2TextureAL rt;
	ASSERT_HRESULT_FAILED( rt.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	EXPECT_FALSE( rt.IsValid() );
}

TEST_F( RenderTarget, RenderTargetIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	EXPECT_TRUE( rt.IsValid() );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
	EXPECT_EQ( 1, rt.GetMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetFormat() );
	EXPECT_EQ( 1, rt.GetMsaaDesc().samples );
	EXPECT_EQ( 0, rt.GetMsaaDesc().quality );
}

TEST_F( RenderTarget, CanCreateMipMappedRenderTarget )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );
	EXPECT_TRUE( rt.IsValid() );
	EXPECT_TRUE( Tr2GpuUsage::HasFlag( rt.GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE ) );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
	EXPECT_EQ( 8, rt.GetTrueMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetFormat() );
	EXPECT_EQ( 1, rt.GetMsaaDesc().samples );
	EXPECT_EQ( 0, rt.GetMsaaDesc().quality );
}

TEST_F( RenderTarget, CanCreateMsaaRenderTarget )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2MsaaDesc( 4 ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	EXPECT_TRUE( rt.IsValid() );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
	EXPECT_EQ( 1, rt.GetMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetFormat() );
	EXPECT_EQ( 4, rt.GetMsaaDesc().samples );
	EXPECT_EQ( 0, rt.GetMsaaDesc().quality );
}

TEST_F( RenderTarget, CanResolveMsaaRenderTarget )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rtMsaa;
	ASSERT_HRESULT_SUCCEEDED( rtMsaa.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2MsaaDesc( 4 ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
	ASSERT_HRESULT_SUCCEEDED( rtMsaa.Resolve( rt, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
}

TEST_F( RenderTarget, RenderTargetEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	EXPECT_TRUE( rt == rt );
}

TEST_F( RenderTarget, DifferentRenderTargetsAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt1;
	ASSERT_HRESULT_SUCCEEDED( rt1.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	Tr2TextureAL rt2;
	ASSERT_HRESULT_SUCCEEDED( rt2.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	EXPECT_FALSE( rt1 == rt2 );
}

TEST_F( RenderTarget, RenderTargetHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );
	auto memoryClass = rt.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( RenderTarget, CanLockRenderTarget )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::READ, *renderContext ) );

	const void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( rt.MapForReading( Tr2TextureSubresource( 0 ), data, pitch, *renderContext ) );
	EXPECT_NE( nullptr, data );
	EXPECT_LE( 4 * 128u, pitch );
	rt.UnmapForReading( *renderContext );
}

TEST_F( RenderTarget, CanLockPartOfRenderTarget )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::READ, *renderContext ) );

	const void* data = nullptr;
	uint32_t pitch = 0;
	uint32_t ltrb[] = { 12, 6, 34, 16 };
	ASSERT_HRESULT_SUCCEEDED( rt.MapForReading( Tr2TextureSubresource( 0 ).SetRect( ltrb ), data, pitch, *renderContext ) );
	EXPECT_NE( nullptr, data );
	EXPECT_LE( 4 * ( ltrb[2] - ltrb[0] ), pitch );
	rt.UnmapForReading( *renderContext );
}

TEST_F( RenderTarget, CanLockRenderTargetMipLevel )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::READ, *renderContext ) );

	const void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( rt.MapForReading( Tr2TextureSubresource( 2 ), data, pitch, *renderContext ) );
	EXPECT_NE( nullptr, data );
	EXPECT_LE( 4 * 32u, pitch );
	rt.UnmapForReading( *renderContext );
}



TEST_F( RenderTarget, CannotCreateCpuReadableMsaaRenderTarget )
{
	Tr2TextureAL rt;
	ASSERT_HRESULT_FAILED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2MsaaDesc( 4 ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::READ, nullptr, *renderContext ) );
}

TEST_F( RenderTarget, CannotCreateCpuWriteableRenderTarget )
{
	Tr2TextureAL rt;
	ASSERT_HRESULT_FAILED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::WRITE, *renderContext ) );
}

TEST_F( RenderTarget, CannotCreateCpuReadableDepthStencil )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_FAILED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, Tr2CpuUsage::READ, *renderContext ) );
}

TEST_F( RenderTarget, CannotCreateCpuWritableDepthStencil )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_FAILED( ds.Create( Tr2BitmapDimensions( 128, 64, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, Tr2CpuUsage::WRITE, *renderContext ) );
}

TEST_F( RenderTarget, CannotCreateDepthStencilWithMips )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_FAILED( ds.Create( Tr2BitmapDimensions( 128, 64, 2, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );
}
