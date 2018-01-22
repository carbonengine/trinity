#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, DepthStencilIsInvalidBeforeCreation )
{
	Tr2TextureAL ds;
	EXPECT_FALSE( ds.IsValid() );
}

TEST_F( WithRenderContext, CreatingDepthStencilWithoutRenderContextFails )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_FAILED( ds.CreateDepthStencil( 128, 128, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_FALSE( ds.IsValid() );
}

TEST_F( WithValidRenderContext, DepthStencilIsValidAfterCreation )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.CreateDepthStencil( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( ConvertDepthStencilFormat( DSFMT_D24S8 ), ds.GetFormat() );
	EXPECT_EQ( 1, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
}

TEST_F( WithValidRenderContext, MsaaDepthStencilIsValidAfterCreation )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.CreateDepthStencil( 128, 64, DSFMT_D24S8, Tr2MsaaDesc( 4 ), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( ConvertDepthStencilFormat( DSFMT_D24S8 ), ds.GetFormat() );
	EXPECT_EQ( 4, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
}

TEST_F( WithValidRenderContext, DepthStencilIsInvalidAfterDestruction )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.CreateDepthStencil( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	ds.Destroy();
	EXPECT_FALSE( ds.IsValid() );
}

TEST_F( WithValidRenderContext, DepthStencilEqualsItself )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.CreateDepthStencil( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds == ds );
}

TEST_F( WithValidRenderContext, DifferentDepthStencilsAreNotEqual )
{
	Tr2TextureAL ds1;
	ASSERT_HRESULT_SUCCEEDED( ds1.CreateDepthStencil( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	Tr2TextureAL ds2;
	ASSERT_HRESULT_SUCCEEDED( ds2.CreateDepthStencil( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_FALSE( ds1 == ds2 );
}

TEST_F( WithValidRenderContext, CanCreateReadableDepthStencil )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.CreateDepthStencil( 128, 64, DSFMT_READABLE, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( ConvertDepthStencilFormat( DSFMT_READABLE ), ds.GetFormat() );
	EXPECT_EQ( 1, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
	EXPECT_TRUE( Tr2GpuUsage::HasFlag( ds.GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE ) );
	EXPECT_EQ( 1, ds.GetMipCount() );
}

TEST_F( WithValidRenderContext, DepthStencilHasMemoryClass )
{
	Tr2TextureAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.CreateDepthStencil( 128, 64, DSFMT_READABLE, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	auto memoryClass = ds.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
