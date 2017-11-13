#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, DepthStencilIsInvalidBeforeCreation )
{
	Tr2DepthStencilAL ds;
	EXPECT_FALSE( ds.IsValid() );
	EXPECT_FALSE( ds.GetTexture().IsValid() );
}

TEST_F( WithRenderContext, CreatingDepthStencilWithoutRenderContextFails )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_FAILED( ds.Create( 128, 128, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_FALSE( ds.IsValid() );
	EXPECT_FALSE( ds.GetTexture().IsValid() );
}

TEST_F( WithValidRenderContext, DepthStencilIsValidAfterCreation )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( DSFMT_D24S8, ds.GetFormat() );
	EXPECT_EQ( 1, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
}

TEST_F( WithValidRenderContext, MsaaDepthStencilIsValidAfterCreation )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc( 4 ), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( DSFMT_D24S8, ds.GetFormat() );
	EXPECT_EQ( 4, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
}

TEST_F( WithValidRenderContext, DepthStencilIsInvalidAfterDestruction )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	ds.Destroy();
	EXPECT_FALSE( ds.IsValid() );
}

TEST_F( WithValidRenderContext, DepthStencilEqualsItself )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds == ds );
}

TEST_F( WithValidRenderContext, DifferentDepthStencilsAreNotEqual )
{
	Tr2DepthStencilAL ds1;
	ASSERT_HRESULT_SUCCEEDED( ds1.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	Tr2DepthStencilAL ds2;
	ASSERT_HRESULT_SUCCEEDED( ds2.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_FALSE( ds1 == ds2 );
}

TEST_F( WithValidRenderContext, CanCreateReadableDepthStencil )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_READABLE, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	EXPECT_TRUE( ds.IsValid() );
	EXPECT_EQ( 128, ds.GetWidth() );
	EXPECT_EQ( 64, ds.GetHeight() );
	EXPECT_EQ( DSFMT_READABLE, ds.GetFormat() );
	EXPECT_EQ( 1, ds.GetMsaaDesc().samples );
	EXPECT_EQ( 0, ds.GetMsaaDesc().quality );
	EXPECT_TRUE( ds.GetTexture().IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, ds.GetTexture().GetType() );
	EXPECT_EQ( 128, ds.GetTexture().GetWidth() );
	EXPECT_EQ( 64, ds.GetTexture().GetHeight() );
	EXPECT_EQ( 1, ds.GetTexture().GetMipCount() );
}

TEST_F( WithValidRenderContext, DepthStencilHasMemoryClass )
{
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_READABLE, Tr2MsaaDesc(), EX_NONE, *renderContext ) );
	auto memoryClass = ds.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
