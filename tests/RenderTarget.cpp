#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, RenderTargetIsInvalidBeforeCreation )
{
	Tr2RenderTargetAL rt;
	EXPECT_FALSE( rt.IsValid() );
	EXPECT_FALSE( rt.GetTexture().IsValid() );
}

TEST_F( WithRenderContext, CreatingRenderTargetWithoutRenderContextFails )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_FAILED( rt.Create( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	EXPECT_FALSE( rt.IsValid() );
	EXPECT_FALSE( rt.GetTexture().IsValid() );
}

TEST_F( WithValidRenderContext, RenderTargetIsValidAfterCreation )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	EXPECT_TRUE( rt.IsValid() );
	EXPECT_TRUE( rt.GetTexture().IsValid() );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
	EXPECT_EQ( 1, rt.GetMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetFormat() );
	EXPECT_EQ( 1, rt.GetMsaaDesc().samples );
	EXPECT_EQ( 0, rt.GetMsaaDesc().quality );
	EXPECT_EQ( 128, rt.GetTexture().GetWidth() );
	EXPECT_EQ( 64, rt.GetTexture().GetHeight() );
	EXPECT_EQ( 1, rt.GetTexture().GetMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetTexture().GetFormat() );
}

TEST_F( WithValidRenderContext, CanCreateMipMappedRenderTarget )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	EXPECT_TRUE( rt.IsValid() );
	EXPECT_TRUE( rt.GetTexture().IsValid() );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
	EXPECT_EQ( 8, rt.GetTrueMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetFormat() );
	EXPECT_EQ( 1, rt.GetMsaaDesc().samples );
	EXPECT_EQ( 0, rt.GetMsaaDesc().quality );
	EXPECT_EQ( 128, rt.GetTexture().GetWidth() );
	EXPECT_EQ( 64, rt.GetTexture().GetHeight() );
	EXPECT_EQ( 8, rt.GetTexture().GetTrueMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetTexture().GetFormat() );
}

TEST_F( WithValidRenderContext, CanCreateMsaaRenderTarget )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc( 4 ), 0, EX_NONE, *renderContext ) );
	EXPECT_TRUE( rt.IsValid() );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
	EXPECT_EQ( 1, rt.GetMipCount() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, rt.GetFormat() );
	EXPECT_EQ( 4, rt.GetMsaaDesc().samples );
	EXPECT_EQ( 0, rt.GetMsaaDesc().quality );
}

TEST_F( WithValidRenderContext, RenderTargetIsInvalidAfterDestruction )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	rt.Destroy();
	EXPECT_FALSE( rt.IsValid() );
}

TEST_F( WithValidRenderContext, CanResolveMsaaRenderTarget )
{
	Tr2RenderTargetAL rtMsaa;
	ASSERT_HRESULT_SUCCEEDED( rtMsaa.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc( 4 ), 0, EX_NONE, *renderContext ) );

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
	ASSERT_HRESULT_SUCCEEDED( rtMsaa.Resolve( rt, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
}

TEST_F( WithValidRenderContext, RenderTargetEqualsItself )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	EXPECT_TRUE( rt == rt );
}

TEST_F( WithValidRenderContext, DifferentRenderTargetsAreNotEqual )
{
	Tr2RenderTargetAL rt1;
	ASSERT_HRESULT_SUCCEEDED( rt1.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	Tr2RenderTargetAL rt2;
	ASSERT_HRESULT_SUCCEEDED( rt2.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	EXPECT_FALSE( rt1 == rt2 );
}

TEST_F( WithValidRenderContext, RenderTargetHasMemoryClass )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );
	auto memoryClass = rt.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( WithValidRenderContext, RenderTargetSurvivesDeviceReset )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	Tr2AutoResetObjectAL::ReleaseALResources();
	renderContext->ReleaseDeviceResources();

	EXPECT_FALSE( rt.IsValid() );

	Tr2AutoResetObjectAL::PrepareALResources( *renderContext );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetPresentParameters( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, presentParameters ) );

	EXPECT_TRUE( rt.IsValid() );
	EXPECT_EQ( 128, rt.GetWidth() );
	EXPECT_EQ( 64, rt.GetHeight() );
}

TEST_F( WithValidRenderContext, CanLockRenderTarget )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( rt.Lock( 0, nullptr, data, pitch, *renderContext ) );
	EXPECT_NE( nullptr, data );
	EXPECT_LE( 4 * 128u, pitch );
	ASSERT_HRESULT_SUCCEEDED( rt.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockPartOfRenderTarget )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	void* data = nullptr;
	uint32_t pitch = 0;
	uint32_t ltrb[] = { 12, 6, 34, 16 };
	ASSERT_HRESULT_SUCCEEDED( rt.Lock( 0, ltrb, data, pitch, *renderContext ) );
	EXPECT_NE( nullptr, data );
	EXPECT_LE( 4 * ( ltrb[2] - ltrb[0] ), pitch );
	ASSERT_HRESULT_SUCCEEDED( rt.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockRenderTargetMipLevel )
{
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( rt.Lock( 2, nullptr, data, pitch, *renderContext ) );
	EXPECT_NE( nullptr, data );
	EXPECT_LE( 4 * 32u, pitch );
	ASSERT_HRESULT_SUCCEEDED( rt.Unlock( *renderContext ) );
}
