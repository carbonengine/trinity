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

#include <dwmapi.h>
#include <cmath>
#include <share.h>

#pragma comment( lib, "dwmapi.lib" )

namespace
{
// Win32 occlusion check (DXGI-independent), mirroring the Tr2MainWindow_Windows.cpp fix exactly: each
// 250ms tick tests a batch of quasirandomly placed points against the opaque top-level windows above us
// in Z-order (one EnumWindows pass per tick). The window only counts as hidden once every sample of
// OCCLUSION_SAMPLES_REQUIRED consecutive samples was covered (or it is minimized); a single visible
// sample resets the run.
const int OCCLUSION_SAMPLES_PER_TICK = 8;
const int OCCLUSION_SAMPLES_REQUIRED = 64; // 8 fully covered ticks (~2s) before the throttle engages

// Sample positions come from the R2 quasirandom sequence (generalised golden ratio phi2, the "plastic
// ratio"): successive points are evenly distributed over the window without aligning to window geometry.
const float INVERSE_PHI2 = 1.0f / 1.32471795724474602596f;
float g_occlusionSeqX = 0.5f;
float g_occlusionSeqY = 0.5f;
int g_occlusionCoveredSamples = 0;

// Suspended UWP/store apps keep a WS_VISIBLE top-level window in the Z-order that draws nothing; without
// this check such a window would count as an occluder and could throttle a perfectly visible client.
bool IsWindowCloaked( HWND hwnd )
{
	DWORD cloaked = 0;
	return SUCCEEDED( ::DwmGetWindowAttribute( hwnd, DWMWA_CLOAKED, &cloaked, sizeof( cloaked ) ) ) && cloaked != 0;
}

// One EnumWindows pass per tick: collect the rects of every opaque, uncloaked window above hwnd in
// Z-order. EnumWindows enumerates topmost first, so collection stops at hwnd - nothing below it can
// occlude it - and each sample point then costs only a few rect tests.
void CollectOccludersAbove( HWND hwnd, std::vector<RECT>& rects )
{
	struct Ctx
	{
		HWND target;
		std::vector<RECT>* rects;
	} ctx = { hwnd, &rects };
	::EnumWindows(
		[]( HWND other, LPARAM lp ) -> BOOL {
			Ctx* c = reinterpret_cast<Ctx*>( lp );
			if( other == c->target )
				return FALSE;
			if( !::IsWindowVisible( other ) || ::IsIconic( other ) )
				return TRUE;
			if( ::GetWindowLongW( other, GWL_EXSTYLE ) & ( WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW ) )
				return TRUE;
			if( IsWindowCloaked( other ) )
				return TRUE;
			RECT r;
			if( ::GetWindowRect( other, &r ) )
				c->rects->push_back( r );
			return TRUE;
		},
		reinterpret_cast<LPARAM>( &ctx ) );
}

// Per-tick occlusion sample; returns whether the window should currently count as hidden. Stateful: the
// covered-sample run accumulates across ticks (see block comment above).
bool SampleWindowOcclusion( HWND hwnd )
{
	if( !hwnd )
	{
		g_occlusionCoveredSamples = 0;
		return false;
	}
	if( ::IsIconic( hwnd ) )
	{
		g_occlusionCoveredSamples = 0;
		return true; // minimized is unambiguous, no sample accumulation needed
	}
	RECT r;
	if( !::GetWindowRect( hwnd, &r ) )
	{
		g_occlusionCoveredSamples = 0;
		return false;
	}
	const int w = r.right - r.left;
	const int h = r.bottom - r.top;
	if( w <= 0 || h <= 0 )
	{
		g_occlusionCoveredSamples = 0;
		return true;
	}

	std::vector<RECT> occluders;
	CollectOccludersAbove( hwnd, occluders );

	for( int i = 0; i < OCCLUSION_SAMPLES_PER_TICK; ++i )
	{
		g_occlusionSeqX += INVERSE_PHI2;
		g_occlusionSeqX -= std::floor( g_occlusionSeqX );
		g_occlusionSeqY += INVERSE_PHI2 * INVERSE_PHI2;
		g_occlusionSeqY -= std::floor( g_occlusionSeqY );
		const POINT pt = { r.left + LONG( g_occlusionSeqX * w ), r.top + LONG( g_occlusionSeqY * h ) };
		bool covered = false;
		for( const RECT& occluder : occluders )
		{
			if( ::PtInRect( &occluder, pt ) )
			{
				covered = true;
				break;
			}
		}
		// A point no occluder covers can still be invisible: outside every monitor (window dragged or
		// stranded off-screen). Count those as covered, as the topmost-window-at-point variant did.
		if( !covered && ::MonitorFromPoint( pt, MONITOR_DEFAULTTONULL ) == nullptr )
			covered = true;
		if( !covered )
		{
			g_occlusionCoveredSamples = 0;
			return false;
		}
	}
	if( g_occlusionCoveredSamples < OCCLUSION_SAMPLES_REQUIRED )
		g_occlusionCoveredSamples += OCCLUSION_SAMPLES_PER_TICK;
	return g_occlusionCoveredSamples >= OCCLUSION_SAMPLES_REQUIRED;
}
}

// Interactive occlusion probe: presents the primary swapchain (bound to the visible 640x480 window) in a
// loop while sampling occlusion exactly like the Tr2MainWindow WINDOW_HIDDEN fix: every 250ms tick, 8
// quasirandom points are tested against the opaque windows above us in Z-order; the throttle engages only
// after 64 consecutive covered samples (~2s fully covered) and releases on the first visible sample.
// While "hidden" it emulates the WINDOW_HIDDEN behaviour (20ms sleep + render 1/50, matching
// TriDevice::Throttle + ShouldSkipFrame). Cover / half-cover / minimize the window and read the log;
// state transitions are logged with the covered-sample counter.
//   TrinityALTest_dx12_internal.exe --interactive --gtest_filter=SwapChain.OcclusionThrottleProbe
TEST_F( SwapChain, OcclusionThrottleProbe )
{
	ENSURE_GPU_OR_SKIP

	// _fsopen with _SH_DENYWR rather than fopen_s (which denies all sharing) so the log can be read
	// and tailed while the probe is running.
	FILE* log = _fsopen( "occlusion_throttle_test.log", "w", _SH_DENYWR );
	if( log )
	{
		fprintf( log, "-- WINDOW_HIDDEN throttle demo (accumulating R2-sampled occlusion, %d samples per "
					  "250ms tick, hidden at %d consecutive covered samples). Covered/minimized -> 20ms "
					  "sleep + render 1/50, matching TriDevice::Throttle + ShouldSkipFrame. "
					  "Click window + press a key to exit --\n",
			OCCLUSION_SAMPLES_PER_TICK, OCCLUSION_SAMPLES_REQUIRED );
		fflush( log );
	}

	LARGE_INTEGER freq;
	QueryPerformanceFrequency( &freq );
	LARGE_INTEGER lastLog;
	QueryPerformanceCounter( &lastLog );
	LARGE_INTEGER lastTick = lastLog;
	LARGE_INTEGER stateSince = lastLog;
	int frame = 0; // loop iterations
	int renders = 0; // actual renders since last log
	unsigned skip = 0;
	uint32_t g = 0;
	bool occluded = false;

	auto body = [&] {
		// Closing the render window ends the probe. The window class uses DefWindowProc, so the close
		// button destroys the HWND but posts no WM_QUIT; without this check the loop would spin forever,
		// with the destroyed window sampling as "visible" (GetWindowRect fails) and thus unthrottled.
		if( !::IsWindow( (HWND)WithWindow::GetWindowHandle() ) )
		{
			if( log )
			{
				fprintf( log, "-- render window closed, exiting --\n" );
				fflush( log );
			}
			::PostQuitMessage( 0 ); // DoLoopProcessing() sees WM_QUIT and ends the RunLoop
			return;
		}

		// Sample on the same 250ms cadence as the Tr2MainWindow occlusion timer.
		LARGE_INTEGER now;
		QueryPerformanceCounter( &now );
		if( double( now.QuadPart - lastTick.QuadPart ) / double( freq.QuadPart ) >= 0.25 )
		{
			lastTick = now;
			const bool nowOccluded = SampleWindowOcclusion( (HWND)WithWindow::GetWindowHandle() );
			if( nowOccluded != occluded )
			{
				if( log )
				{
					const double inState = double( now.QuadPart - stateSince.QuadPart ) / double( freq.QuadPart );
					fprintf( log, "== transition %s -> %s (%.2fs in previous state, coveredSamples=%d)\n",
						occluded ? "OCCLUDED" : "visible", nowOccluded ? "OCCLUDED" : "visible", inState,
						g_occlusionCoveredSamples );
					fflush( log );
				}
				stateSince = now;
			}
			occluded = nowOccluded;
		}

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
			LARGE_INTEGER logNow;
			QueryPerformanceCounter( &logNow );
			const double elapsed = double( logNow.QuadPart - lastLog.QuadPart ) / double( freq.QuadPart );
			lastLog = logNow;
			const double loopFps = elapsed > 0.0 ? 30.0 / elapsed : 0.0;
			const double renderFps = elapsed > 0.0 ? renders / elapsed : 0.0;
			renders = 0;
			fprintf( log, "frame %6d | %-8s throttle=%-3s | samples=%2d | loopFps=%9.1f | renderFps=%8.1f\n", frame, occluded ? "OCCLUDED" : "visible", occluded ? "ON" : "off", g_occlusionCoveredSamples, loopFps, renderFps );
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
