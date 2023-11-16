#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct Fence : public WithValidRenderContext
{
};


TEST_F( Fence, FenceIsInvalidBeforeCreation )
{
	Tr2FenceAL fence;
	EXPECT_FALSE( fence.IsValid() );
}

TEST_F( WithRenderContext, CreatingFenceWithoutRenderContextFails )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_FAILED( fence.Create( *renderContext ) );
	EXPECT_FALSE( fence.IsValid() );
}

TEST_F( Fence, FenceIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	EXPECT_TRUE( fence.IsValid() );
}

TEST_F( Fence, UsingInvalidFenceFails )
{
	Tr2FenceAL fence;

	ASSERT_HRESULT_FAILED( fence.PutFence( *renderContext ) );

	bool reached = false;
	ASSERT_HRESULT_FAILED( fence.IsReached( reached, *renderContext ) );
}

TEST_F( Fence, CanUseFence )
{
	ENSURE_GPU_OR_SKIP
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	ASSERT_TRUE( fence.IsValid() );

	ASSERT_HRESULT_SUCCEEDED( fence.PutFence( *renderContext ) );
	bool reached = false;
	ASSERT_HRESULT_SUCCEEDED( fence.IsReached( reached, *renderContext ) );
}

TEST_F( Fence, FenceEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	EXPECT_TRUE( fence == fence );
}

TEST_F( Fence, DifferentFencesAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2FenceAL fence1;
	ASSERT_HRESULT_SUCCEEDED( fence1.Create( *renderContext ) );
	Tr2FenceAL fence2;
	ASSERT_HRESULT_SUCCEEDED( fence2.Create( *renderContext ) );
	EXPECT_FALSE( fence1 == fence2 );
}

TEST_F( Fence, FenceHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	auto memoryClass = fence.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
