#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct OcclusionQuery : public WithValidRenderContext
{
};


TEST_F( OcclusionQuery, OcclusionQueryIsInvalidBeforeCreation )
{
	Tr2OcclusionQueryAL query;
	EXPECT_FALSE( query.IsValid() );
}

TEST_F( WithRenderContext, CreatingOcclusionQueryWithoutRenderContextFails )
{
	Tr2OcclusionQueryAL query;
	ASSERT_HRESULT_FAILED( query.Create( *renderContext ) );
	EXPECT_FALSE( query.IsValid() );
}

TEST_F( OcclusionQuery, OcclusionQueryIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2OcclusionQueryAL query;
	ASSERT_HRESULT_SUCCEEDED( query.Create( *renderContext ) );
	EXPECT_TRUE( query.IsValid() );
}

TEST_F( OcclusionQuery, RuningInvalidOcclusionQueryFails )
{
	Tr2OcclusionQueryAL query;

	ASSERT_HRESULT_FAILED( query.Begin( *renderContext ) );
	ASSERT_HRESULT_FAILED( query.End( *renderContext ) );
	uint32_t count = 0;
	ASSERT_HRESULT_FAILED( query.GetPixelCount( *renderContext, count, Tr2OcclusionQueryAL::WAIT ) );
}

TEST_F( OcclusionQuery, CanRunOcclusionQuery )
{
	ENSURE_GPU_OR_SKIP
	Tr2OcclusionQueryAL query;
	ASSERT_HRESULT_SUCCEEDED( query.Create( *renderContext ) );
	ASSERT_TRUE( query.IsValid() );

	ASSERT_HRESULT_SUCCEEDED( query.Begin( *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( query.End( *renderContext ) );
}

TEST_F( OcclusionQuery, CanGetOcclusionQueryResultSynchronously )
{
	ENSURE_GPU_OR_SKIP
	Tr2OcclusionQueryAL query;
	ASSERT_HRESULT_SUCCEEDED( query.Create( *renderContext ) );
	ASSERT_TRUE( query.IsValid() );

	ASSERT_HRESULT_SUCCEEDED( query.Begin( *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( query.End( *renderContext ) );

	uint32_t count = 0;
	ASSERT_HRESULT_SUCCEEDED( query.GetPixelCount( *renderContext, count, Tr2OcclusionQueryAL::WAIT ) );
	EXPECT_EQ( 0, count );
}

TEST_F( OcclusionQuery, OcclusionQueryEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2OcclusionQueryAL query;
	ASSERT_HRESULT_SUCCEEDED( query.Create( *renderContext ) );
	EXPECT_TRUE( query == query );
}

TEST_F( OcclusionQuery, DifferentOcclusionQueriesAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2OcclusionQueryAL query1;
	ASSERT_HRESULT_SUCCEEDED( query1.Create( *renderContext ) );
	Tr2OcclusionQueryAL query2;
	ASSERT_HRESULT_SUCCEEDED( query2.Create( *renderContext ) );
	EXPECT_FALSE( query1 == query2 );
}

TEST_F( OcclusionQuery, OcclusionQueryHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2OcclusionQueryAL query;
	ASSERT_HRESULT_SUCCEEDED( query.Create( *renderContext ) );
	auto memoryClass = query.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
