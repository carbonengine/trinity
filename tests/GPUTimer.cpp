#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

#if TRINITY_PLATFORM != TRINITY_STUB 

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, TimerIsInvalidBeforeCreation )
{
	Tr2GpuTimerAL timer;
	EXPECT_FALSE(timer.IsValid());
}

TEST_F( WithRenderContext, CreatingWithoutRenderContext )
{
	Tr2GpuTimerAL timer;
	auto hrResult = timer.Create(*renderContext);
	ASSERT_HRESULT_FAILED(hrResult);
}

TEST_F( WithValidRenderContext, IsValidAfterCreation )
{
	Tr2GpuTimerAL timer;
	auto hrResult = timer.Create(*renderContext);
	ASSERT_HRESULT_SUCCEEDED(hrResult);
	EXPECT_TRUE(timer.IsValid());
}

TEST_F( WithValidRenderContext, Begins )
{
	Tr2GpuTimerAL timer;
	const auto hrResult = timer.Create(*renderContext);
	ASSERT_HRESULT_SUCCEEDED(hrResult);
	const auto begins = timer.Begin(*renderContext);
	EXPECT_TRUE(begins);
}

TEST_F( WithValidRenderContext, ValidAfterStopping )
{
	Tr2GpuTimerAL timer;
	const auto hrResult = timer.Create(*renderContext);
	ASSERT_HRESULT_SUCCEEDED(hrResult);
	timer.Begin(*renderContext);
	timer.End(*renderContext);
	EXPECT_TRUE(timer.IsValid());
}

#endif