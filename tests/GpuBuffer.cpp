#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

TEST_F( WithValidRenderContext, GpuBufferIsInvalidBeforeCreation )
{
	Tr2GpuBufferAL vb;
	EXPECT_FALSE( vb.IsValid() );
}

TEST_F( WithRenderContext, CreatingGpuBufferWithoutRenderContextFails )
{
	Tr2GpuBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingImmutableGpuBufferWithoutInitialDataFails )
{
	Tr2GpuBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_IMMUTABLE, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanCreateTypedGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
		EXPECT_EQ( 128 * 4, vb.GetTotalSizeInBytes() );
		EXPECT_EQ( Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, vb.GetFormat() );
	}
	else
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_FALSE( vb.IsValid() );
		EXPECT_EQ( 0, vb.GetTotalSizeInBytes() );
		EXPECT_EQ( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN, vb.GetFormat() );
	}
}

TEST_F( WithValidRenderContext, CanCreateStructuredGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, 32, 0, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
	}
	else
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_FAILED( vb.Create( 128, 32, 0, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_FALSE( vb.IsValid() );
	}
}

TEST_F( WithValidRenderContext, CanCreateAppendConsumeStructuredGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, 32, 0, Tr2RenderContextEnum::GPU_BUFFER_APPEND, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
	}
}

TEST_F( WithValidRenderContext, CanCreateStructuredGpuBufferWithCounter )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, 32, 0, Tr2RenderContextEnum::GPU_BUFFER_COUNTER, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
	}
}

TEST_F( WithValidRenderContext, GpuBufferIsInvalidAfterDestruction )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		vb.Destroy();
		EXPECT_FALSE( vb.IsValid() );
	}
}

TEST_F( WithValidRenderContext, GpuBufferReportsCorrectSizeAndType )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
		EXPECT_EQ( 128, vb.GetNumElements() );
		EXPECT_EQ( 4, vb.BytesPerElement() );
		EXPECT_EQ( 128 * 4, vb.GetTotalSizeInBytes() );
		EXPECT_EQ( Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, vb.GetFormat() );
	}
}

TEST_F( WithValidRenderContext, StructuredGpuBufferReportsUnknownFormat )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, 32, 0, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
		EXPECT_EQ( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN, vb.GetFormat() );
	}
}

TEST_F( WithValidRenderContext, GpuBufferEqualsItself )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_TRUE( vb == vb );
	}
}

TEST_F( WithValidRenderContext, DifferentGpuBuffersAreNotEqual )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb1;
		ASSERT_HRESULT_SUCCEEDED( vb1.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		Tr2GpuBufferAL vb2;
		ASSERT_HRESULT_SUCCEEDED( vb2.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		EXPECT_FALSE( vb1 == vb2 );
	}
}

TEST_F( WithValidRenderContext, LockingInvalidGpuBufferFails )
{
	Tr2GpuBufferAL vb;
	void* data;
	ASSERT_HRESULT_FAILED( vb.Lock( 0, 0, &data, Tr2RenderContextEnum::LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_FAILED( vb.Lock( 0, 0, &data, Tr2RenderContextEnum::LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockGpuBufferForReading )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_READ, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		void* data;
		ASSERT_HRESULT_SUCCEEDED( vb.Lock( 0, 0, &data, Tr2RenderContextEnum::LOCK_READONLY, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( vb.Unlock( *renderContext ) );
	}
}

TEST_F( WithValidRenderContext, CanLockGpuBufferForWriting )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_WRITE, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		void* data;
		ASSERT_HRESULT_SUCCEEDED( vb.Lock( 0, 0, &data, Tr2RenderContextEnum::LOCK_WRITEONLY, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( vb.Unlock( *renderContext ) );
	}
}

TEST_F( WithValidRenderContext, GpuBufferHasMemoryClass )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_READ, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		auto memoryClass = vb.GetMemoryClass();
		EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
	}
}

TEST_F( WithValidRenderContext, CanCopyGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb1;
		ASSERT_HRESULT_SUCCEEDED( vb1.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		Tr2GpuBufferAL vb2;
		ASSERT_HRESULT_SUCCEEDED( vb2.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, Tr2RenderContextEnum::EX_NONE, nullptr, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( vb1.CopySubBuffer( 0, 4, vb2, 16, *renderContext ) );
	}
}

TEST_F( WithValidRenderContext, CanPassInitialDataToGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		float original[] = { 1.f, 2.f, 3.f, 4.f };

		Tr2GpuBufferAL buffer;
		ASSERT_HRESULT_SUCCEEDED( buffer.Create( 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_READ, Tr2RenderContextEnum::EX_NONE, original, *renderContext ) );

		float* data;
		ASSERT_HRESULT_SUCCEEDED( buffer.Lock( 0, 0, (void**)&data, Tr2RenderContextEnum::LOCK_READONLY, *renderContext ) );
		EXPECT_EQ( original[0], data[0] );
		EXPECT_EQ( original[1], data[1] );
		EXPECT_EQ( original[2], data[2] );
		EXPECT_EQ( original[3], data[3] );
		ASSERT_HRESULT_SUCCEEDED( buffer.Unlock( *renderContext ) );
	}
}
