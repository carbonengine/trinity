#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

#if _WIN32 || _WIN64
#if _WIN64
#define PLATFORM_64BIT
#else
#define PLATFORM_32BIT
#endif
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define PLATFORM_64BIT
#else
#define PLATFORM_32BIT
#endif
#endif

struct ConstantBuffer : public WithValidRenderContext
{
};


TEST_F( ConstantBuffer, ConstantBufferIsInvalidBeforeCreation )
{
	Tr2ConstantBufferAL vb;
	EXPECT_FALSE( vb.IsValid() );
}

TEST_F( WithRenderContext, CreatingConstantBufferWithoutRenderContextFails )
{
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, *renderContext ) );
}

TEST_F( ConstantBuffer, CreatingImmutableConstantBufferWithoutInitialDataFails )
{
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2ConstantUsageAL::IMMUTABLE, nullptr, *renderContext ) );
}

TEST_F( ConstantBuffer, ConstantBufferIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	EXPECT_TRUE( vb.IsValid() );
}

TEST_F( ConstantBuffer, ConstantBufferReportsCorrectSize )
{
	ENSURE_GPU_OR_SKIP
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	EXPECT_EQ( 128, vb.GetSize() );
}

TEST_F( ConstantBuffer, ConstantBufferEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	EXPECT_TRUE( vb == vb );
}

TEST_F( ConstantBuffer, DifferentConstantBuffersAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2ConstantBufferAL vb1;
	ASSERT_HRESULT_SUCCEEDED( vb1.Create( 128, *renderContext ) );
	Tr2ConstantBufferAL vb2;
	ASSERT_HRESULT_SUCCEEDED( vb2.Create( 128, *renderContext ) );
	EXPECT_FALSE( vb1 == vb2 );
}

TEST_F( ConstantBuffer, LockingInvalidConstantBufferFails )
{
	Tr2ConstantBufferAL vb;
	void* data;
	ASSERT_HRESULT_FAILED( vb.Lock( &data, *renderContext ) );
}

TEST_F( ConstantBuffer, CanLockConstantBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	void* data;
	ASSERT_HRESULT_SUCCEEDED( vb.Lock( &data, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( vb.Unlock( *renderContext ) );
}

#if( TRINITYPLATFORM == TRINITY_STUB )
TEST_F( ConstantBuffer, UnlockingConstantBufferThatHasNotBeenLockedFails )
{
	Tr2ConstantBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	ASSERT_HRESULT_FAILED( vb.Unlock( *renderContext ) );
}


TEST_F( ConstantBuffer, LockingImmutableConstantBufferFails )
{
	Tr2ConstantBufferAL vb;
	char initialData[128];
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::USAGE_IMMUTABLE, initialData, *renderContext ) );
	void* data;
	ASSERT_HRESULT_FAILED( vb.Lock( &data, *renderContext ) );
}
#endif

TEST_F( ConstantBuffer, ConstantBufferHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( 128, *renderContext ) );
	auto memoryClass = cb.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( ConstantBuffer, CanCreateConstantBufferWithInitialData )
{
	ENSURE_GPU_OR_SKIP
	float initialData[] = { 0.5f, 1.5f, 2.5f, 3.5f };
	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( initialData ), Tr2ConstantUsageAL::REUSABLE, initialData, *renderContext ) );
	// we can't really test if initial data is set correctly since CBs are write-only for CPU
}

#ifndef PLATFORM_64BIT
TEST_F( ConstantBuffer, CreatingTooLargeConstantBufferFails )
{
	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_FAILED( cb.Create( uint32_t( -1 ), *renderContext ) );
}
#endif