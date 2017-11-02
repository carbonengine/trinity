#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, CanSetViewport )
{
	Tr2Viewport viewport( 123, 67 );
	viewport.m_x = 30;
	viewport.m_y = 18;
	viewport.m_minZ = 0.1f;
	viewport.m_maxZ = 0.6f;
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetViewport( viewport ) );
	Tr2Viewport gotViewport;
	ASSERT_HRESULT_SUCCEEDED( renderContext->GetViewport( gotViewport ) );
	EXPECT_EQ( viewport.m_x, gotViewport.m_x );
	EXPECT_EQ( viewport.m_y, gotViewport.m_y );
	EXPECT_EQ( viewport.m_width, gotViewport.m_width );
	EXPECT_EQ( viewport.m_height, gotViewport.m_height );
	EXPECT_EQ( viewport.m_minZ, gotViewport.m_minZ );
	EXPECT_EQ( viewport.m_maxZ, gotViewport.m_maxZ );
}

TEST_F( WithValidRenderContext, CanGetBackbufferFormat )
{
	EXPECT_NE( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN, renderContext->GetBackBufferFormat() );
}

TEST_F( WithValidRenderContext, CanGetBackbufferSize )
{
	uint32_t width = 0xDeadBeef;
	uint32_t height = 0xDeadBeef;
	ASSERT_HRESULT_SUCCEEDED( renderContext->GetRenderTargetSize( width, height ) );
	EXPECT_NE( 0xDeadBeef, width );
	EXPECT_NE( 0xDeadBeef, height );
}

TEST_F( WithValidRenderContext, CanGetRenderTargetSize )
{
	uint32_t width = 0xDeadBeef;
	uint32_t height = 0xDeadBeef;

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->GetRenderTargetSize( width, height ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

	EXPECT_EQ( rt.GetWidth(), width );
	EXPECT_EQ( rt.GetHeight(), height );
}

TEST_F( WithValidRenderContext, CanGetRenderTargetSizeForNonZeroSlot )
{
	uint32_t width = 0xDeadBeef;
	uint32_t height = 0xDeadBeef;
	const uint32_t slot = 2;

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget( slot ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt, slot ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->GetRenderTargetSize( width, height, slot ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget( slot ) );

	EXPECT_EQ( rt.GetWidth(), width );
	EXPECT_EQ( rt.GetHeight(), height );
}

TEST_F( WithValidRenderContext, GetRenderTargetSizeFailsWithNoRenderTarget )
{
	uint32_t width = 0xDeadBeef;
	uint32_t height = 0xDeadBeef;
	const uint32_t slot = 1;

	ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget( slot ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( nullRT, slot ) );
	ASSERT_HRESULT_FAILED( renderContext->GetRenderTargetSize( width, height, slot ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget( slot ) );

	EXPECT_EQ( 0xDeadBeef, width );
	EXPECT_EQ( 0xDeadBeef, height );
}

TEST_F( WithRenderContext, InvalidRenderContextHasInvalidBackBuffer )
{
	EXPECT_FALSE( renderContext->GetDefaultBackBuffer().IsValid() );
}

TEST_F( WithValidRenderContext, ValidRenderContextHasValidBackBuffer )
{
	EXPECT_TRUE( renderContext->GetDefaultBackBuffer().IsValid() );
	EXPECT_EQ( 1, renderContext->GetDefaultBackBuffer().GetMipCount() );
	EXPECT_EQ( renderContext->GetBackBufferFormat(), renderContext->GetDefaultBackBuffer().GetFormat() );
}

TEST_F( WithValidRenderContext, SettingInvalidClipPlaneSucceeds )
{
	// Setting clip plane with invalid index shuold be ignored silently
	float plane[] = { 1.0f, 2.0f, 3.0f, 4.0f };
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetClipPlane( 128, plane ) );
}

TEST_F( WithValidRenderContext, CanSetNumberOfLights )
{
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetNumberOfLights( 13 ) );
}

TEST_F( WithValidRenderContext, CanConvertPixelFormatToTypeless )
{
	Tr2RenderContextEnum::PixelFormat formats[] = {
		PIXEL_FORMAT_UNKNOWN	                 , PIXEL_FORMAT_UNKNOWN	                 ,
		PIXEL_FORMAT_R32G32B32A32_TYPELESS       , PIXEL_FORMAT_R32G32B32A32_TYPELESS       ,
		PIXEL_FORMAT_R32G32B32A32_FLOAT          , PIXEL_FORMAT_R32G32B32A32_TYPELESS          ,
		PIXEL_FORMAT_R32G32B32A32_UINT           , PIXEL_FORMAT_R32G32B32A32_TYPELESS           ,
		PIXEL_FORMAT_R32G32B32A32_SINT           , PIXEL_FORMAT_R32G32B32A32_TYPELESS           ,
		PIXEL_FORMAT_R32G32B32_TYPELESS          , PIXEL_FORMAT_R32G32B32_TYPELESS          ,
		PIXEL_FORMAT_R32G32B32_FLOAT             , PIXEL_FORMAT_R32G32B32_TYPELESS             ,
		PIXEL_FORMAT_R32G32B32_UINT              , PIXEL_FORMAT_R32G32B32_TYPELESS              ,
		PIXEL_FORMAT_R32G32B32_SINT              , PIXEL_FORMAT_R32G32B32_TYPELESS              ,
		PIXEL_FORMAT_R16G16B16A16_TYPELESS       , PIXEL_FORMAT_R16G16B16A16_TYPELESS       ,
		PIXEL_FORMAT_R16G16B16A16_FLOAT          , PIXEL_FORMAT_R16G16B16A16_TYPELESS          ,
		PIXEL_FORMAT_R16G16B16A16_UNORM          , PIXEL_FORMAT_R16G16B16A16_TYPELESS          ,
		PIXEL_FORMAT_R16G16B16A16_UINT           , PIXEL_FORMAT_R16G16B16A16_TYPELESS           ,
		PIXEL_FORMAT_R16G16B16A16_SNORM          , PIXEL_FORMAT_R16G16B16A16_TYPELESS          ,
		PIXEL_FORMAT_R16G16B16A16_SINT           , PIXEL_FORMAT_R16G16B16A16_TYPELESS           ,
		PIXEL_FORMAT_R32G32_TYPELESS             , PIXEL_FORMAT_R32G32_TYPELESS             ,
		PIXEL_FORMAT_R32G32_FLOAT                , PIXEL_FORMAT_R32G32_TYPELESS                ,
		PIXEL_FORMAT_R32G32_UINT                 , PIXEL_FORMAT_R32G32_TYPELESS                 ,
		PIXEL_FORMAT_R32G32_SINT                 , PIXEL_FORMAT_R32G32_TYPELESS                 ,
		PIXEL_FORMAT_R32G8X24_TYPELESS           , PIXEL_FORMAT_R32G8X24_TYPELESS           ,
		PIXEL_FORMAT_D32_FLOAT_S8X24_UINT        , PIXEL_FORMAT_D32_FLOAT_S8X24_UINT        ,
		PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS    , PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS    ,
		PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT     , PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT     ,
		PIXEL_FORMAT_R10G10B10A2_TYPELESS        , PIXEL_FORMAT_R10G10B10A2_TYPELESS        ,
		PIXEL_FORMAT_R10G10B10A2_UNORM           , PIXEL_FORMAT_R10G10B10A2_TYPELESS           ,
		PIXEL_FORMAT_R10G10B10A2_UINT            , PIXEL_FORMAT_R10G10B10A2_TYPELESS            ,
		PIXEL_FORMAT_R11G11B10_FLOAT             , PIXEL_FORMAT_R11G11B10_FLOAT             ,
		PIXEL_FORMAT_R8G8B8A8_TYPELESS           , PIXEL_FORMAT_R8G8B8A8_TYPELESS           ,
		PIXEL_FORMAT_R8G8B8A8_UNORM              , PIXEL_FORMAT_R8G8B8A8_TYPELESS              ,
		PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB         , PIXEL_FORMAT_R8G8B8A8_TYPELESS         ,
		PIXEL_FORMAT_R8G8B8A8_UINT               , PIXEL_FORMAT_R8G8B8A8_TYPELESS               ,
		PIXEL_FORMAT_R8G8B8A8_SNORM              , PIXEL_FORMAT_R8G8B8A8_TYPELESS              ,
		PIXEL_FORMAT_R8G8B8A8_SINT               , PIXEL_FORMAT_R8G8B8A8_TYPELESS               ,
		PIXEL_FORMAT_R16G16_TYPELESS             , PIXEL_FORMAT_R16G16_TYPELESS             ,
		PIXEL_FORMAT_R16G16_FLOAT                , PIXEL_FORMAT_R16G16_TYPELESS                ,
		PIXEL_FORMAT_R16G16_UNORM                , PIXEL_FORMAT_R16G16_TYPELESS                ,
		PIXEL_FORMAT_R16G16_UINT                 , PIXEL_FORMAT_R16G16_TYPELESS                 ,
		PIXEL_FORMAT_R16G16_SNORM                , PIXEL_FORMAT_R16G16_TYPELESS                ,
		PIXEL_FORMAT_R16G16_SINT                 , PIXEL_FORMAT_R16G16_TYPELESS                 ,
		PIXEL_FORMAT_R32_TYPELESS                , PIXEL_FORMAT_R32_TYPELESS                ,
		PIXEL_FORMAT_D32_FLOAT                   , PIXEL_FORMAT_D32_FLOAT                   ,
		PIXEL_FORMAT_R32_FLOAT                   , PIXEL_FORMAT_R32_TYPELESS                   ,
		PIXEL_FORMAT_R32_UINT                    , PIXEL_FORMAT_R32_TYPELESS                    ,
		PIXEL_FORMAT_R32_SINT                    , PIXEL_FORMAT_R32_TYPELESS                    ,
		PIXEL_FORMAT_R24G8_TYPELESS              , PIXEL_FORMAT_R24G8_TYPELESS              ,
		PIXEL_FORMAT_D24_UNORM_S8_UINT           , PIXEL_FORMAT_D24_UNORM_S8_UINT           ,
		PIXEL_FORMAT_R24_UNORM_X8_TYPELESS       , PIXEL_FORMAT_R24_UNORM_X8_TYPELESS       ,
		PIXEL_FORMAT_X24_TYPELESS_G8_UINT        , PIXEL_FORMAT_X24_TYPELESS_G8_UINT        ,
		PIXEL_FORMAT_R8G8_TYPELESS               , PIXEL_FORMAT_R8G8_TYPELESS               ,
		PIXEL_FORMAT_R8G8_UNORM                  , PIXEL_FORMAT_R8G8_TYPELESS                  ,
		PIXEL_FORMAT_R8G8_UINT                   , PIXEL_FORMAT_R8G8_TYPELESS                   ,
		PIXEL_FORMAT_R8G8_SNORM                  , PIXEL_FORMAT_R8G8_TYPELESS                  ,
		PIXEL_FORMAT_R8G8_SINT                   , PIXEL_FORMAT_R8G8_TYPELESS                   ,
		PIXEL_FORMAT_R16_TYPELESS                , PIXEL_FORMAT_R16_TYPELESS                ,
		PIXEL_FORMAT_R16_FLOAT                   , PIXEL_FORMAT_R16_TYPELESS                   ,
		PIXEL_FORMAT_D16_UNORM                   , PIXEL_FORMAT_D16_UNORM                   ,
		PIXEL_FORMAT_R16_UNORM                   , PIXEL_FORMAT_R16_TYPELESS                   ,
		PIXEL_FORMAT_R16_UINT                    , PIXEL_FORMAT_R16_TYPELESS                    ,
		PIXEL_FORMAT_R16_SNORM                   , PIXEL_FORMAT_R16_TYPELESS                   ,
		PIXEL_FORMAT_R16_SINT                    , PIXEL_FORMAT_R16_TYPELESS                    ,
		PIXEL_FORMAT_R8_TYPELESS                 , PIXEL_FORMAT_R8_TYPELESS                 ,
		PIXEL_FORMAT_R8_UNORM                    , PIXEL_FORMAT_R8_TYPELESS                    ,
		PIXEL_FORMAT_R8_UINT                     , PIXEL_FORMAT_R8_TYPELESS                     ,
		PIXEL_FORMAT_R8_SNORM                    , PIXEL_FORMAT_R8_TYPELESS                    ,
		PIXEL_FORMAT_R8_SINT                     , PIXEL_FORMAT_R8_TYPELESS                     ,
		PIXEL_FORMAT_A8_UNORM                    , PIXEL_FORMAT_A8_UNORM                    ,
		PIXEL_FORMAT_R1_UNORM                    , PIXEL_FORMAT_R1_UNORM                    ,
		PIXEL_FORMAT_R9G9B9E5_SHAREDEXP          , PIXEL_FORMAT_R9G9B9E5_SHAREDEXP          ,
		PIXEL_FORMAT_R8G8_B8G8_UNORM             , PIXEL_FORMAT_R8G8_B8G8_UNORM             ,
		PIXEL_FORMAT_G8R8_G8B8_UNORM             , PIXEL_FORMAT_G8R8_G8B8_UNORM             ,
		PIXEL_FORMAT_BC1_TYPELESS                , PIXEL_FORMAT_BC1_TYPELESS                ,
		PIXEL_FORMAT_BC1_UNORM                   , PIXEL_FORMAT_BC1_TYPELESS                   ,
		PIXEL_FORMAT_BC1_UNORM_SRGB              , PIXEL_FORMAT_BC1_TYPELESS              ,
		PIXEL_FORMAT_BC2_TYPELESS                , PIXEL_FORMAT_BC2_TYPELESS                ,
		PIXEL_FORMAT_BC2_UNORM                   , PIXEL_FORMAT_BC2_TYPELESS                   ,
		PIXEL_FORMAT_BC2_UNORM_SRGB              , PIXEL_FORMAT_BC2_TYPELESS              ,
		PIXEL_FORMAT_BC3_TYPELESS                , PIXEL_FORMAT_BC3_TYPELESS                ,
		PIXEL_FORMAT_BC3_UNORM                   , PIXEL_FORMAT_BC3_TYPELESS                   ,
		PIXEL_FORMAT_BC3_UNORM_SRGB              , PIXEL_FORMAT_BC3_TYPELESS              ,
		PIXEL_FORMAT_BC4_TYPELESS                , PIXEL_FORMAT_BC4_TYPELESS                ,
		PIXEL_FORMAT_BC4_UNORM                   , PIXEL_FORMAT_BC4_TYPELESS                   ,
		PIXEL_FORMAT_BC4_SNORM                   , PIXEL_FORMAT_BC4_TYPELESS                   ,
		PIXEL_FORMAT_BC5_TYPELESS                , PIXEL_FORMAT_BC5_TYPELESS                ,
		PIXEL_FORMAT_BC5_UNORM                   , PIXEL_FORMAT_BC5_TYPELESS                   ,
		PIXEL_FORMAT_BC5_SNORM                   , PIXEL_FORMAT_BC5_TYPELESS                   ,
		PIXEL_FORMAT_B5G6R5_UNORM                , PIXEL_FORMAT_B5G6R5_UNORM                ,
		PIXEL_FORMAT_B5G5R5A1_UNORM              , PIXEL_FORMAT_B5G5R5A1_UNORM              ,
		PIXEL_FORMAT_B8G8R8A8_UNORM              , PIXEL_FORMAT_B8G8R8A8_TYPELESS              ,
		PIXEL_FORMAT_B8G8R8X8_UNORM              , PIXEL_FORMAT_B8G8R8X8_TYPELESS              ,
		PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  , PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  ,
		PIXEL_FORMAT_B8G8R8A8_TYPELESS           , PIXEL_FORMAT_B8G8R8A8_TYPELESS           ,
		PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB         , PIXEL_FORMAT_B8G8R8A8_TYPELESS         ,
		PIXEL_FORMAT_B8G8R8X8_TYPELESS           , PIXEL_FORMAT_B8G8R8X8_TYPELESS           ,
		PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB         , PIXEL_FORMAT_B8G8R8X8_TYPELESS         ,
		PIXEL_FORMAT_BC6H_TYPELESS               , PIXEL_FORMAT_BC6H_TYPELESS               ,
		PIXEL_FORMAT_BC6H_UF16                   , PIXEL_FORMAT_BC6H_TYPELESS                   ,
		PIXEL_FORMAT_BC6H_SF16                   , PIXEL_FORMAT_BC6H_TYPELESS                   ,
		PIXEL_FORMAT_BC7_TYPELESS                , PIXEL_FORMAT_BC7_TYPELESS                ,
		PIXEL_FORMAT_BC7_UNORM                   , PIXEL_FORMAT_BC7_TYPELESS                   ,
		PIXEL_FORMAT_BC7_UNORM_SRGB              , PIXEL_FORMAT_BC7_TYPELESS };

	for( uint32_t i = 0; i < sizeof( formats ) / sizeof( formats[0] ); i += 2 )
	{
		EXPECT_EQ( formats[i + 1], Tr2RenderContextEnum::MakeTypeless( formats[i] ) );
	}
}

TEST_F( WithValidRenderContext, CanConvertPixelFormatTosRgb )
{
	Tr2RenderContextEnum::PixelFormat formats[] = {
		PIXEL_FORMAT_UNKNOWN	                 , PIXEL_FORMAT_UNKNOWN	                 ,
		PIXEL_FORMAT_R32G32B32A32_TYPELESS       , PIXEL_FORMAT_R32G32B32A32_TYPELESS       ,
		PIXEL_FORMAT_R32G32B32A32_FLOAT          , PIXEL_FORMAT_R32G32B32A32_FLOAT          ,
		PIXEL_FORMAT_R32G32B32A32_UINT           , PIXEL_FORMAT_R32G32B32A32_UINT           ,
		PIXEL_FORMAT_R32G32B32A32_SINT           , PIXEL_FORMAT_R32G32B32A32_SINT           ,
		PIXEL_FORMAT_R32G32B32_TYPELESS          , PIXEL_FORMAT_R32G32B32_TYPELESS          ,
		PIXEL_FORMAT_R32G32B32_FLOAT             , PIXEL_FORMAT_R32G32B32_FLOAT             ,
		PIXEL_FORMAT_R32G32B32_UINT              , PIXEL_FORMAT_R32G32B32_UINT              ,
		PIXEL_FORMAT_R32G32B32_SINT              , PIXEL_FORMAT_R32G32B32_SINT              ,
		PIXEL_FORMAT_R16G16B16A16_TYPELESS       , PIXEL_FORMAT_R16G16B16A16_TYPELESS       ,
		PIXEL_FORMAT_R16G16B16A16_FLOAT          , PIXEL_FORMAT_R16G16B16A16_FLOAT          ,
		PIXEL_FORMAT_R16G16B16A16_UNORM          , PIXEL_FORMAT_R16G16B16A16_UNORM          ,
		PIXEL_FORMAT_R16G16B16A16_UINT           , PIXEL_FORMAT_R16G16B16A16_UINT           ,
		PIXEL_FORMAT_R16G16B16A16_SNORM          , PIXEL_FORMAT_R16G16B16A16_SNORM          ,
		PIXEL_FORMAT_R16G16B16A16_SINT           , PIXEL_FORMAT_R16G16B16A16_SINT           ,
		PIXEL_FORMAT_R32G32_TYPELESS             , PIXEL_FORMAT_R32G32_TYPELESS             ,
		PIXEL_FORMAT_R32G32_FLOAT                , PIXEL_FORMAT_R32G32_FLOAT                ,
		PIXEL_FORMAT_R32G32_UINT                 , PIXEL_FORMAT_R32G32_UINT                 ,
		PIXEL_FORMAT_R32G32_SINT                 , PIXEL_FORMAT_R32G32_SINT                 ,
		PIXEL_FORMAT_R32G8X24_TYPELESS           , PIXEL_FORMAT_R32G8X24_TYPELESS           ,
		PIXEL_FORMAT_D32_FLOAT_S8X24_UINT        , PIXEL_FORMAT_D32_FLOAT_S8X24_UINT        ,
		PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS    , PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS    ,
		PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT     , PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT     ,
		PIXEL_FORMAT_R10G10B10A2_TYPELESS        , PIXEL_FORMAT_R10G10B10A2_TYPELESS        ,
		PIXEL_FORMAT_R10G10B10A2_UNORM           , PIXEL_FORMAT_R10G10B10A2_UNORM           ,
		PIXEL_FORMAT_R10G10B10A2_UINT            , PIXEL_FORMAT_R10G10B10A2_UINT            ,
		PIXEL_FORMAT_R11G11B10_FLOAT             , PIXEL_FORMAT_R11G11B10_FLOAT             ,
		PIXEL_FORMAT_R8G8B8A8_TYPELESS           , PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB         ,
		PIXEL_FORMAT_R8G8B8A8_UNORM              , PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB         ,
		PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB         , PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB         ,
		PIXEL_FORMAT_R8G8B8A8_UINT               , PIXEL_FORMAT_R8G8B8A8_UINT               ,
		PIXEL_FORMAT_R8G8B8A8_SNORM              , PIXEL_FORMAT_R8G8B8A8_SNORM              ,
		PIXEL_FORMAT_R8G8B8A8_SINT               , PIXEL_FORMAT_R8G8B8A8_SINT               ,
		PIXEL_FORMAT_R16G16_TYPELESS             , PIXEL_FORMAT_R16G16_TYPELESS             ,
		PIXEL_FORMAT_R16G16_FLOAT                , PIXEL_FORMAT_R16G16_FLOAT                ,
		PIXEL_FORMAT_R16G16_UNORM                , PIXEL_FORMAT_R16G16_UNORM                ,
		PIXEL_FORMAT_R16G16_UINT                 , PIXEL_FORMAT_R16G16_UINT                 ,
		PIXEL_FORMAT_R16G16_SNORM                , PIXEL_FORMAT_R16G16_SNORM                ,
		PIXEL_FORMAT_R16G16_SINT                 , PIXEL_FORMAT_R16G16_SINT                 ,
		PIXEL_FORMAT_R32_TYPELESS                , PIXEL_FORMAT_R32_TYPELESS                ,
		PIXEL_FORMAT_D32_FLOAT                   , PIXEL_FORMAT_D32_FLOAT                   ,
		PIXEL_FORMAT_R32_FLOAT                   , PIXEL_FORMAT_R32_FLOAT                   ,
		PIXEL_FORMAT_R32_UINT                    , PIXEL_FORMAT_R32_UINT                    ,
		PIXEL_FORMAT_R32_SINT                    , PIXEL_FORMAT_R32_SINT                    ,
		PIXEL_FORMAT_R24G8_TYPELESS              , PIXEL_FORMAT_R24G8_TYPELESS              ,
		PIXEL_FORMAT_D24_UNORM_S8_UINT           , PIXEL_FORMAT_D24_UNORM_S8_UINT           ,
		PIXEL_FORMAT_R24_UNORM_X8_TYPELESS       , PIXEL_FORMAT_R24_UNORM_X8_TYPELESS       ,
		PIXEL_FORMAT_X24_TYPELESS_G8_UINT        , PIXEL_FORMAT_X24_TYPELESS_G8_UINT        ,
		PIXEL_FORMAT_R8G8_TYPELESS               , PIXEL_FORMAT_R8G8_TYPELESS               ,
		PIXEL_FORMAT_R8G8_UNORM                  , PIXEL_FORMAT_R8G8_UNORM                  ,
		PIXEL_FORMAT_R8G8_UINT                   , PIXEL_FORMAT_R8G8_UINT                   ,
		PIXEL_FORMAT_R8G8_SNORM                  , PIXEL_FORMAT_R8G8_SNORM                  ,
		PIXEL_FORMAT_R8G8_SINT                   , PIXEL_FORMAT_R8G8_SINT                   ,
		PIXEL_FORMAT_R16_TYPELESS                , PIXEL_FORMAT_R16_TYPELESS                ,
		PIXEL_FORMAT_R16_FLOAT                   , PIXEL_FORMAT_R16_FLOAT                   ,
		PIXEL_FORMAT_D16_UNORM                   , PIXEL_FORMAT_D16_UNORM                   ,
		PIXEL_FORMAT_R16_UNORM                   , PIXEL_FORMAT_R16_UNORM                   ,
		PIXEL_FORMAT_R16_UINT                    , PIXEL_FORMAT_R16_UINT                    ,
		PIXEL_FORMAT_R16_SNORM                   , PIXEL_FORMAT_R16_SNORM                   ,
		PIXEL_FORMAT_R16_SINT                    , PIXEL_FORMAT_R16_SINT                    ,
		PIXEL_FORMAT_R8_TYPELESS                 , PIXEL_FORMAT_R8_TYPELESS                 ,
		PIXEL_FORMAT_R8_UNORM                    , PIXEL_FORMAT_R8_UNORM                    ,
		PIXEL_FORMAT_R8_UINT                     , PIXEL_FORMAT_R8_UINT                     ,
		PIXEL_FORMAT_R8_SNORM                    , PIXEL_FORMAT_R8_SNORM                    ,
		PIXEL_FORMAT_R8_SINT                     , PIXEL_FORMAT_R8_SINT                     ,
		PIXEL_FORMAT_A8_UNORM                    , PIXEL_FORMAT_A8_UNORM                    ,
		PIXEL_FORMAT_R1_UNORM                    , PIXEL_FORMAT_R1_UNORM                    ,
		PIXEL_FORMAT_R9G9B9E5_SHAREDEXP          , PIXEL_FORMAT_R9G9B9E5_SHAREDEXP          ,
		PIXEL_FORMAT_R8G8_B8G8_UNORM             , PIXEL_FORMAT_R8G8_B8G8_UNORM             ,
		PIXEL_FORMAT_G8R8_G8B8_UNORM             , PIXEL_FORMAT_G8R8_G8B8_UNORM             ,
		PIXEL_FORMAT_BC1_TYPELESS                , PIXEL_FORMAT_BC1_UNORM_SRGB              ,
		PIXEL_FORMAT_BC1_UNORM                   , PIXEL_FORMAT_BC1_UNORM_SRGB              ,
		PIXEL_FORMAT_BC1_UNORM_SRGB              , PIXEL_FORMAT_BC1_UNORM_SRGB              ,
		PIXEL_FORMAT_BC2_TYPELESS                , PIXEL_FORMAT_BC2_UNORM_SRGB              ,
		PIXEL_FORMAT_BC2_UNORM                   , PIXEL_FORMAT_BC2_UNORM_SRGB              ,
		PIXEL_FORMAT_BC2_UNORM_SRGB              , PIXEL_FORMAT_BC2_UNORM_SRGB              ,
		PIXEL_FORMAT_BC3_TYPELESS                , PIXEL_FORMAT_BC3_UNORM_SRGB              ,
		PIXEL_FORMAT_BC3_UNORM                   , PIXEL_FORMAT_BC3_UNORM_SRGB              ,
		PIXEL_FORMAT_BC3_UNORM_SRGB              , PIXEL_FORMAT_BC3_UNORM_SRGB              ,
		PIXEL_FORMAT_BC4_TYPELESS                , PIXEL_FORMAT_BC4_TYPELESS                ,
		PIXEL_FORMAT_BC4_UNORM                   , PIXEL_FORMAT_BC4_UNORM                   ,
		PIXEL_FORMAT_BC4_SNORM                   , PIXEL_FORMAT_BC4_SNORM                   ,
		PIXEL_FORMAT_BC5_TYPELESS                , PIXEL_FORMAT_BC5_TYPELESS                ,
		PIXEL_FORMAT_BC5_UNORM                   , PIXEL_FORMAT_BC5_UNORM                   ,
		PIXEL_FORMAT_BC5_SNORM                   , PIXEL_FORMAT_BC5_SNORM                   ,
		PIXEL_FORMAT_B5G6R5_UNORM                , PIXEL_FORMAT_B5G6R5_UNORM                ,
		PIXEL_FORMAT_B5G5R5A1_UNORM              , PIXEL_FORMAT_B5G5R5A1_UNORM              ,
		PIXEL_FORMAT_B8G8R8A8_UNORM              , PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB         ,
		PIXEL_FORMAT_B8G8R8X8_UNORM              , PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB         ,
		PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  , PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  ,
		PIXEL_FORMAT_B8G8R8A8_TYPELESS           , PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB         ,
		PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB         , PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB         ,
		PIXEL_FORMAT_B8G8R8X8_TYPELESS           , PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB         ,
		PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB         , PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB         ,
		PIXEL_FORMAT_BC6H_TYPELESS               , PIXEL_FORMAT_BC6H_TYPELESS               ,
		PIXEL_FORMAT_BC6H_UF16                   , PIXEL_FORMAT_BC6H_UF16                   ,
		PIXEL_FORMAT_BC6H_SF16                   , PIXEL_FORMAT_BC6H_SF16                   ,
		PIXEL_FORMAT_BC7_TYPELESS                , PIXEL_FORMAT_BC7_UNORM_SRGB              ,
		PIXEL_FORMAT_BC7_UNORM                   , PIXEL_FORMAT_BC7_UNORM_SRGB              ,
		PIXEL_FORMAT_BC7_UNORM_SRGB              , PIXEL_FORMAT_BC7_UNORM_SRGB              , };

	for( uint32_t i = 0; i < sizeof( formats ) / sizeof( formats[0] ); i += 2 )
	{
		EXPECT_EQ( formats[i + 1], Tr2RenderContextEnum::MakeSrgb( formats[i] ) );
	}
}

TEST_F( WithValidRenderContext, PrimaryRenderContextIsSetCorrectly )
{
	EXPECT_EQ( renderContext, &renderContext->GetPrimaryRenderContext() );
}
