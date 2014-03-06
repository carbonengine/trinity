#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, IndexBufferIsInvalidBeforeCreation )
{
	Tr2IndexBufferAL ib;
	EXPECT_FALSE( ib.IsValid() );
}

TEST_F( WithRenderContext, CreatingIndexBufferWithoutRenderContextFails )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_FAILED( ib.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingImmutableIndexBufferWithoutInitialDataFails )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_FAILED( ib.Create( 128, USAGE_IMMUTABLE, IB_16BIT, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, IndexBufferIsValidAfterCreation )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	EXPECT_TRUE( ib.IsValid() );
}

TEST_F( WithValidRenderContext, IndexBufferIsInvalidAfterDestruction )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	ib.Destroy();
	EXPECT_FALSE( ib.IsValid() );
}

TEST_F( WithValidRenderContext, IndexBufferReportsCorrectSize )
{
	Tr2IndexBufferAL ib1;
	ASSERT_HRESULT_SUCCEEDED( ib1.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	EXPECT_EQ( 128, ib1.GetNumIndices() );
	EXPECT_EQ( 128 * 2, ib1.GetTotalSizeInBytes() );
	Tr2IndexBufferAL ib2;
	ASSERT_HRESULT_SUCCEEDED( ib2.Create( 128, 0, IB_32BIT, nullptr, *renderContext ) );
	EXPECT_EQ( 128, ib2.GetNumIndices() );
	EXPECT_EQ( 128 * 4, ib2.GetTotalSizeInBytes() );
}

TEST_F( WithValidRenderContext, IndexBufferReportsCorrectBitCount )
{
	Tr2IndexBufferAL ib16;
	ASSERT_HRESULT_SUCCEEDED( ib16.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	EXPECT_EQ( 2, ib16.BytesPerIndex() );
	EXPECT_EQ( IB_16BIT, ib16.GetIBBitcount() );
	Tr2IndexBufferAL ib32;
	ASSERT_HRESULT_SUCCEEDED( ib32.Create( 128, 0, IB_32BIT, nullptr, *renderContext ) );
	EXPECT_EQ( 4, ib32.BytesPerIndex() );
	EXPECT_EQ( IB_32BIT, ib32.GetIBBitcount() );
}

TEST_F( WithValidRenderContext, IndexBufferEqualsItself )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	EXPECT_TRUE( ib == ib );
}

TEST_F( WithValidRenderContext, DifferentIndexBuffersAreNotEqual )
{
	Tr2IndexBufferAL ib1;
	ASSERT_HRESULT_SUCCEEDED( ib1.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	Tr2IndexBufferAL ib2;
	ASSERT_HRESULT_SUCCEEDED( ib2.Create( 128, 0, IB_16BIT, nullptr, *renderContext ) );
	EXPECT_FALSE( ib1 == ib2 );
}

TEST_F( WithValidRenderContext, LockingInvalidIndexBufferFails )
{
	Tr2IndexBufferAL ib;
	void* data;
	ASSERT_HRESULT_FAILED( ib.Lock( 0, 16, &data, LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_FAILED( ib.Lock( 0, 16, &data, LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockIndexBufferForReading )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_16BIT, nullptr, *renderContext ) );
	void* data;
	ASSERT_HRESULT_SUCCEEDED( ib.Lock( 0, 16, &data, LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockIndexBufferForWriting )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_16BIT, nullptr, *renderContext ) );
	void* data;
	ASSERT_HRESULT_SUCCEEDED( ib.Lock( 0, 16, &data, LOCK_WRITEONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockDynamicIndexBufferWithNoOverwrite )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE | USAGE_LOCK_FREQUENTLY, IB_16BIT, nullptr, *renderContext ) );
	void* data;
	if( renderContext->GetCaps().SupportsNoOverwriteLock() )
	{
		ASSERT_HRESULT_SUCCEEDED( ib.Lock( 0, 16, &data, LOCK_NO_OVERWRITE, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
	}
	else
	{
		ASSERT_HRESULT_FAILED( ib.Lock( 0, 16, &data, LOCK_NO_OVERWRITE, *renderContext ) );
	}
}

TEST_F( WithValidRenderContext, CanNotLockIndexBufferWithNoOverwrite )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_16BIT, nullptr, *renderContext ) );
	void* data;
	ASSERT_HRESULT_FAILED( ib.Lock( 0, 16, &data, LOCK_NO_OVERWRITE, *renderContext ) );
}

TEST_F( WithValidRenderContext, IndexBufferHasMemoryClass )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_16BIT, nullptr, *renderContext ) );
	auto memoryClass = ib.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( WithValidRenderContext, CanMoveIndexBuffer )
{
	Tr2IndexBufferAL ib1;
	ASSERT_HRESULT_SUCCEEDED( ib1.Create( 128, USAGE_CPU_WRITE, IB_16BIT, nullptr, *renderContext ) );

	Tr2IndexBufferAL ib2;
	ib2 = std::move( ib1 );

	EXPECT_FALSE( ib1.IsValid() );
	EXPECT_TRUE( ib2.IsValid() );
	EXPECT_EQ( 128, ib2.GetNumIndices() );
}

TEST_F( WithValidRenderContext, CreatingManagedImmutableIndexBufferFails )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_FAILED( ib.Create( 128, USAGE_IMMUTABLE | USAGE_HINT_MANAGED, IB_16BIT, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingReadableDynamicIndexBufferFails )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_FAILED( ib.Create( 128, USAGE_CPU_READ | USAGE_LOCK_FREQUENTLY, IB_16BIT, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanCreateDynamicIndexBuffer )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_LOCK_FREQUENTLY, IB_16BIT, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanCreateIndexBufferTwice )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_16BIT, nullptr, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 256, USAGE_CPU_READ, IB_16BIT, nullptr, *renderContext ) );
	EXPECT_EQ( 256, ib.GetNumIndices() );
}

TEST_F( WithValidRenderContext, CanLock16BitIndexBufferForReading )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_16BIT, nullptr, *renderContext ) );
	uint16_t* data;
	ASSERT_HRESULT_SUCCEEDED( ib.Lock( data, LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanNotLock16BitIndexBufferWith32BitIndicesForReading )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_16BIT, nullptr, *renderContext ) );
	uint32_t* data;
	ASSERT_HRESULT_FAILED( ib.Lock( data, LOCK_READONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLock32BitIndexBufferForReading )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_32BIT, nullptr, *renderContext ) );
	uint32_t* data;
	ASSERT_HRESULT_SUCCEEDED( ib.Lock( data, LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanNotLock32BitIndexBufferWith16BitIndicesForReading )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_32BIT, nullptr, *renderContext ) );
	uint16_t* data;
	ASSERT_HRESULT_FAILED( ib.Lock( data, LOCK_READONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLock16BitIndexBufferForWriting )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_16BIT, nullptr, *renderContext ) );
	uint16_t* data;
	ASSERT_HRESULT_SUCCEEDED( ib.Lock( data, LOCK_WRITEONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanNotLock16BitIndexBufferWith32BitIndicesForWriting )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_16BIT, nullptr, *renderContext ) );
	uint32_t* data;
	ASSERT_HRESULT_FAILED( ib.Lock( data, LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLock32BitIndexBufferForWriting )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_32BIT, nullptr, *renderContext ) );
	uint32_t* data;
	ASSERT_HRESULT_SUCCEEDED( ib.Lock( data, LOCK_WRITEONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanNotLock32BitIndexBufferWith16BitIndicesForWriting )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_32BIT, nullptr, *renderContext ) );
	uint16_t* data;
	ASSERT_HRESULT_FAILED( ib.Lock( data, LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanNotLockReadOnlyIndexBufferForWriting )
{
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_16BIT, nullptr, *renderContext ) );
	void* data;
	ASSERT_HRESULT_FAILED( ib.Lock( 0, 16, &data, LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, UpdatingInvalidIndexBufferFails )
{
	Tr2IndexBufferAL ib;
	uint32_t data[128] = { 0, };
	EXPECT_HRESULT_FAILED( ib.UpdateBuffer( 0, 128, data, *renderContext ) );
}

TEST_F( WithValidRenderContext, UpdatingImmutableIndexBufferFails )
{
	uint32_t data[128] = { 0, };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_IMMUTABLE, IB_32BIT, data, *renderContext ) );
	EXPECT_HRESULT_FAILED( ib.UpdateBuffer( 0, 64, data, *renderContext ) );
}

TEST_F( WithValidRenderContext, UpdatingNonWritableIndexBufferFails )
{
	uint32_t data[128] = { 0, };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_READ, IB_32BIT, nullptr, *renderContext ) );
	EXPECT_HRESULT_FAILED( ib.UpdateBuffer( 0, 64, data, *renderContext ) );
}

TEST_F( WithValidRenderContext, UpdatingDynamicIndexBufferFails )
{
	uint32_t data[128] = { 0, };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE | USAGE_LOCK_FREQUENTLY, IB_32BIT, nullptr, *renderContext ) );
	EXPECT_HRESULT_FAILED( ib.UpdateBuffer( 0, 64, data, *renderContext ) );
}

TEST_F( WithValidRenderContext, UpdatingIndexBufferOutOfBoundsFails )
{
	uint32_t data[128] = { 0, };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_32BIT, nullptr, *renderContext ) );
	EXPECT_HRESULT_FAILED( ib.UpdateBuffer( ib.GetTotalSizeInBytes() - 32, 128, data, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanUpdateIndexBuffer )
{
	uint32_t data[128] = { 0, };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, USAGE_CPU_WRITE, IB_32BIT, nullptr, *renderContext ) );
	EXPECT_HRESULT_SUCCEEDED( ib.UpdateBuffer( 0, ib.GetTotalSizeInBytes(), data, *renderContext ) );
	EXPECT_HRESULT_SUCCEEDED( ib.UpdateBuffer( 64, 32, data, *renderContext ) );
}

