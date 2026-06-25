// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct GpuTimer : public WithValidRenderContext
{
};


TEST_F( GpuTimer, TimerIsInvalidBeforeCreation )
{
	Tr2GpuTimerAL timer;
	EXPECT_FALSE( timer.IsValid() );
}

TEST_F( WithRenderContext, CreatingWithoutRenderContext )
{
	Tr2GpuTimerAL timer;
	auto hrResult = timer.Create( *renderContext );
	ASSERT_HRESULT_FAILED( hrResult );
}

// GPU timers are disabled for now on Metal because of stability issues
#if TRINITY_PLATFORM != TRINITY_METAL

TEST_F( GpuTimer, IsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2GpuTimerAL timer;
	auto hrResult = timer.Create( *renderContext );
	ASSERT_HRESULT_SUCCEEDED( hrResult );
	EXPECT_TRUE( timer.IsValid() );
}

TEST_F( GpuTimer, Begins )
{
	ENSURE_GPU_OR_SKIP
	Tr2GpuTimerAL timer;
	const auto hrResult = timer.Create( *renderContext );
	ASSERT_HRESULT_SUCCEEDED( hrResult );
	const auto begins = timer.Begin( *renderContext );
	EXPECT_TRUE( begins );
}

TEST_F( GpuTimer, ValidAfterStopping )
{
	ENSURE_GPU_OR_SKIP
	Tr2GpuTimerAL timer;
	const auto hrResult = timer.Create( *renderContext );
	ASSERT_HRESULT_SUCCEEDED( hrResult );
	timer.Begin( *renderContext );
	timer.End( *renderContext );
	EXPECT_TRUE( timer.IsValid() );
}

#endif
