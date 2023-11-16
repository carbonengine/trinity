#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"

struct Buffer : public WithValidRenderContext
{
};

TEST_F( Buffer, BufferIsInvalidBeforeCreation )
{
	Tr2BufferAL buffer;
	EXPECT_FALSE( buffer.IsValid() );
}

TEST_F( Buffer, CreatingImmutableBufferWithoutInitialDataFails )
{
	Tr2BufferAL buffer;
	ASSERT_HRESULT_FAILED( buffer.Create( 1, 128, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, nullptr, *renderContext ) );
}

TEST_F( Buffer, BufferIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	float data[4] = { 0 };
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, *renderContext ) );
	EXPECT_TRUE( buffer.IsValid() );
}

TEST_F( Buffer, BufferReportsCorrectSize )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	float data[4] = { 0 };
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, *renderContext ) );
	EXPECT_EQ( sizeof( data ), buffer.GetSize() );
}

TEST_F( Buffer, BufferEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	float data[4] = { 0 };
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, *renderContext ) );
	EXPECT_TRUE( buffer == buffer );
}

TEST_F( Buffer, DifferentBuffersAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	float data[4] = { 0 };
	Tr2BufferAL buffer1;
	ASSERT_HRESULT_SUCCEEDED( buffer1.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, *renderContext ) );
	Tr2BufferAL buffer2;
	ASSERT_HRESULT_SUCCEEDED( buffer2.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, *renderContext ) );

	EXPECT_FALSE( buffer1 == buffer2 );
}

TEST_F( Buffer, ReadingInvalidBufferFails )
{
	Tr2BufferAL buffer;
	const void* data;
	ASSERT_HRESULT_FAILED( buffer.MapForReading( data, *renderContext ) );
}

TEST_F( Buffer, WritingInvalidBufferFails )
{
	Tr2BufferAL buffer;
	void* data;
	ASSERT_HRESULT_FAILED( buffer.MapForWriting( data, *renderContext ) );
}

TEST_F( Buffer, UpdatingInvalidBufferFails )
{
	Tr2BufferAL buffer;
	float data;
	ASSERT_HRESULT_FAILED( buffer.UpdateBuffer( 0, sizeof( float ), &data, *renderContext ) );
}

TEST_F( Buffer, CanLockReadableBufferForReading )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	float data[4] = { 1, 2, 3, 4 };
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data, *renderContext ) );

	const float* locked;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForReading( locked, *renderContext ) );
#if TRINITY_PLATFORM != TRINITY_STUB
	EXPECT_EQ( 1, locked[0] );
	EXPECT_EQ( 2, locked[1] );
	EXPECT_EQ( 3, locked[2] );
	EXPECT_EQ( 4, locked[3] );
#endif
	buffer.UnmapForReading( *renderContext );
}

TEST_F( Buffer, LockingNonReadableBufferForReadingFails )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	float data[4] = { 0 };
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, *renderContext ) );

	const float* locked;
	EXPECT_HRESULT_FAILED( buffer.MapForReading( locked, *renderContext ) );
	buffer.UnmapForReading( *renderContext );
}

TEST_F( Buffer, CanLockWriteableBufferForWriting )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );

	float* locked;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForWriting( locked, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );
}

TEST_F( Buffer, LockingNonWriteableBufferForWritingFails )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	float data[4] = { 0 };
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data, *renderContext ) );

	float* locked;
	EXPECT_HRESULT_FAILED( buffer.MapForWriting( locked, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );
}

TEST_F( Buffer, CanLockDynamicBufferForWriting )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, nullptr, *renderContext ) );

	float* locked;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForWriting( locked, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );
}

TEST_F( Buffer, CanLockDynamicBufferForWritingWithoutSynchronization )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, nullptr, *renderContext ) );

	float* locked;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForWriting( locked, Tr2LockType::NON_SYNCHRONIZED, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );
}

TEST_F( Buffer, CanUpdateBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );

	float data = 0;
	ASSERT_HRESULT_SUCCEEDED( buffer.UpdateBuffer( 0, sizeof( data ), &data, *renderContext ) );
}

TEST_F( Buffer, CanCreate16BitIndexBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( 2, 4, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );
}

TEST_F( Buffer, CanCreate32BitIndexBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( 2, 4, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );
}

#if TRINITY_PLATFORM_SUPPORTS_BUFFER_SHADER_RESOURCES

TEST_F( Buffer, CanCreateStructuredShaderResourceBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( 64, 4, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );
}

TEST_F( Buffer, CanCreateTypedShaderResourceBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 4, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );
}

TEST_F( Buffer, CanCreateVertexBufferWithShaderResource )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( 64, 4, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, *renderContext ) );
}

TEST_F( Buffer, CanCreateDynamicSrvBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( 64, 4, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, nullptr, *renderContext ) );
}

TEST_F( Buffer, CanMapDynamicSrvBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( 64, 4, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, nullptr, *renderContext ) );
	void* data;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForWriting( data, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );

	ASSERT_HRESULT_SUCCEEDED( buffer.MapForWriting( data, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );
}

#if TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS


TEST_F( Buffer, CanCreateWritableShaderResourceBuffer )
{
	ENSURE_GPU_OR_SKIP
	Tr2BufferAL buffer;
	ASSERT_HRESULT_SUCCEEDED( buffer.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 4, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::NONE, nullptr, *renderContext ) );
}

#endif


#endif