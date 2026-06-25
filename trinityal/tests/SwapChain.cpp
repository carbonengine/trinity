// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"
#include "RenderWindow.h"

struct SwapChain : public WithValidRenderContext
{
};

TEST_F( SwapChain, SwapChainIsInvalidBeforeCreation )
{
	Tr2SwapChainAL sc;
	EXPECT_FALSE( sc.IsValid() );
}

TEST_F( WithRenderContext, CreatingSwapChainWithoutRenderContextFails )
{
	RenderWindow window;
	Tr2SwapChainAL sc;
	ASSERT_HRESULT_FAILED( sc.Create( window, *renderContext ) );
}

TEST_F( SwapChain, CanCreateSwapChain )
{
	ENSURE_GPU_OR_SKIP
	if( renderContext->GetCaps().SupportsStandaloneSwapChain() )
	{
		RenderWindow window;
		Tr2SwapChainAL sc;
		ASSERT_HRESULT_SUCCEEDED( sc.Create( window, *renderContext ) );
		EXPECT_TRUE( sc.IsValid() );
		EXPECT_TRUE( sc.GetBackBuffer().IsValid() );
#if ( TRINITY_PLATFORM != TRINITY_STUB )
		EXPECT_EQ( window.GetClientWidth(), sc.GetWidth() );
		EXPECT_EQ( window.GetClientHeight(), sc.GetHeight() );
		EXPECT_EQ( sc.GetWidth(), sc.GetBackBuffer().GetWidth() );
		EXPECT_EQ( sc.GetHeight(), sc.GetBackBuffer().GetHeight() );
#endif
	}
}

TEST_F( SwapChain, SwapChainEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	if( renderContext->GetCaps().SupportsStandaloneSwapChain() )
	{
		RenderWindow window;
		Tr2SwapChainAL sc;
		ASSERT_HRESULT_SUCCEEDED( sc.Create( window, *renderContext ) );
		EXPECT_TRUE( sc == sc );
	}
}

TEST_F( SwapChain, DifferentSwapChainsAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	if( renderContext->GetCaps().SupportsStandaloneSwapChain() )
	{
		RenderWindow window1;
		Tr2SwapChainAL sc1;
		ASSERT_HRESULT_SUCCEEDED( sc1.Create( window1, *renderContext ) );

		RenderWindow window2;
		Tr2SwapChainAL sc2;
		ASSERT_HRESULT_SUCCEEDED( sc2.Create( window2, *renderContext ) );

		EXPECT_FALSE( sc1 == sc2 );
	}
}

TEST_F( SwapChain, SwapChainHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	if( renderContext->GetCaps().SupportsStandaloneSwapChain() )
	{
		RenderWindow window;
		Tr2SwapChainAL sc;
		ASSERT_HRESULT_SUCCEEDED( sc.Create( window, *renderContext ) );

		auto memoryClass = sc.GetMemoryClass();
		EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
	}
}

TEST_F( SwapChain, CanPresentSwapChain )
{
	ENSURE_GPU_OR_SKIP
	if( renderContext->GetCaps().SupportsStandaloneSwapChain() )
	{
		RenderWindow window;
		Tr2SwapChainAL sc;
		ASSERT_HRESULT_SUCCEEDED( sc.Create( window, *renderContext ) );

		uint32_t g = 127;

		auto frame = [&] {
			ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );

			ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( sc.GetBackBuffer() ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
			ASSERT_HRESULT_SUCCEEDED( sc.Present( *renderContext ) );

			ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 16 ), 1.0f ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
			MakeTestScreenShot();
			ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
			g++;
		};

		RunLoop( frame );
	}
}

TEST_F( SwapChain, CanRecreateSwapChain )
{
	ENSURE_GPU_OR_SKIP
	if( renderContext->GetCaps().SupportsStandaloneSwapChain() )
	{
		RenderWindow window;

		uint32_t g = 127;

		auto frame = [&] {
			Tr2SwapChainAL sc;
			ASSERT_HRESULT_SUCCEEDED( sc.Create( window, *renderContext ) );

			ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );

			ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( sc.GetBackBuffer() ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
			ASSERT_HRESULT_SUCCEEDED( sc.Present( *renderContext ) );

			ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 16 ), 1.0f ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
			MakeTestScreenShot();
			ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
			g++;
		};

		RunLoop( frame );
	}
}
