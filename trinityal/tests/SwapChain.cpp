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

namespace
{
// Win32 occlusion check (DXGI-independent): true if the window is minimized, or every sampled point is
// covered by another opaque top-level window. Mirrors the validated Python prototype: sample 5 points
// (centre + 4 inset corners), and at each find the topmost OPAQUE window (skipping see-through overlays:
// WS_EX_LAYERED / WS_EX_TRANSPARENT / WS_EX_TOOLWINDOW, e.g. the Alt+Tab switcher).
bool PointTopIsWindow( HWND target, int px, int py )
{
	struct Ctx
	{
		int px, py;
		HWND result;
	} ctx = { px, py, nullptr };
	::EnumWindows(
		[]( HWND hwnd, LPARAM lp ) -> BOOL {
			Ctx* c = reinterpret_cast<Ctx*>( lp );
			if( !::IsWindowVisible( hwnd ) || ::IsIconic( hwnd ) )
				return TRUE;
			if( ::GetWindowLongW( hwnd, GWL_EXSTYLE ) & ( WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW ) )
				return TRUE;
			RECT r;
			if( !::GetWindowRect( hwnd, &r ) )
				return TRUE;
			if( c->px >= r.left && c->px < r.right && c->py >= r.top && c->py < r.bottom )
			{
				c->result = hwnd; // topmost opaque window covering this point
				return FALSE;
			}
			return TRUE;
		},
		reinterpret_cast<LPARAM>( &ctx ) );
	return ctx.result == target;
}

bool IsWindowOccluded( HWND hwnd )
{
	if( !hwnd )
		return false;
	if( ::IsIconic( hwnd ) )
		return true; // minimized
	RECT r;
	if( !::GetWindowRect( hwnd, &r ) )
		return false;
	const int w = r.right - r.left;
	const int h = r.bottom - r.top;
	if( w <= 0 || h <= 0 )
		return true;
	const double fx[5] = { 0.5, 0.12, 0.88, 0.12, 0.88 };
	const double fy[5] = { 0.5, 0.12, 0.12, 0.88, 0.88 };
	for( int i = 0; i < 5; ++i )
	{
		if( PointTopIsWindow( hwnd, r.left + int( fx[i] * w ), r.top + int( fy[i] * h ) ) )
			return false; // our window is visible at this point
	}
	return true; // every sampled point covered by another opaque window
}
}

// Interactive occlusion probe: presents the primary swapchain (bound to the visible 640x480 window) in a
// loop and logs the DXGI Present HRESULT + fps. Cover / minimize / half-cover the window and read the log;
// DXGI_STATUS_OCCLUDED should appear only when the window is fully covered or minimized.
//   TrinityALTest_dx12_internal.exe --interactive --gtest_filter=SwapChain.OcclusionThrottleProbe
TEST_F( SwapChain, OcclusionThrottleProbe )
{
	ENSURE_GPU_OR_SKIP

	FILE* log = nullptr;
	fopen_s( &log, "occlusion_throttle_test.log", "w" );
	if( log )
	{
		fprintf( log, "-- WINDOW_HIDDEN throttle demo: covered/minimized -> Win32-occlusion-gated throttle "
					  "(20ms sleep + render 1/50, matching TriDevice::Throttle + ShouldSkipFrame). "
					  "Click window + press a key to exit --\n" );
		fflush( log );
	}

	LARGE_INTEGER freq;
	QueryPerformanceFrequency( &freq );
	LARGE_INTEGER lastLog;
	QueryPerformanceCounter( &lastLog );
	int frame = 0; // loop iterations
	int renders = 0; // actual renders since last log
	unsigned skip = 0;
	uint32_t g = 0;

	auto body = [&] {
		const bool occluded = IsWindowOccluded( (HWND)WithWindow::GetWindowHandle() );

		// Emulate TriDevice::ShouldSkipFrame() for WINDOW_HIDDEN: render only 1 frame in 50.
		bool doRender = true;
		if( occluded )
			doRender = ( ( skip++ % 50 ) == 0 );
		else
			skip = 0;

		if( doRender )
		{
			ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( renderContext->GetDefaultBackBuffer() ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
			ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
			++renders;
		}

		// Emulate TriDevice::Throttle() sleep for WINDOW_HIDDEN (20ms).
		if( occluded )
			::Sleep( 20 );

		++frame;
		++g;

		if( frame % 30 == 0 && log )
		{
			LARGE_INTEGER now;
			QueryPerformanceCounter( &now );
			const double elapsed = double( now.QuadPart - lastLog.QuadPart ) / double( freq.QuadPart );
			lastLog = now;
			const double loopFps = elapsed > 0.0 ? 30.0 / elapsed : 0.0;
			const double renderFps = elapsed > 0.0 ? renders / elapsed : 0.0;
			renders = 0;
			fprintf( log, "frame %6d | %-8s throttle=%-3s | loopFps=%9.1f | renderFps=%8.1f\n", frame, occluded ? "OCCLUDED" : "visible", occluded ? "ON" : "off", loopFps, renderFps );
			fflush( log );
		}
	};

	RunLoop( body );

	if( log )
	{
		fclose( log );
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
