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
	ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingImmutableGpuBufferWithoutInitialDataFails )
{
	Tr2GpuBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_IMMUTABLE, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanCreateTypedGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
		EXPECT_EQ( 128 * 4, vb.GetTotalSizeInBytes() );
		EXPECT_EQ( Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, vb.GetFormat() );
	}
	else
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_FAILED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
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
		ASSERT_HRESULT_SUCCEEDED( vb.CreateStructured( 128, 32, 0, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
	}
	else
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_FAILED( vb.CreateStructured( 128, 32, 0, nullptr, *renderContext ) );
		EXPECT_FALSE( vb.IsValid() );
	}
}

TEST_F( WithValidRenderContext, CanCreateGpuBufferAlias )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL buffer1;
		ASSERT_HRESULT_SUCCEEDED( buffer1.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		Tr2GpuBufferAL buffer2;
		ASSERT_HRESULT_SUCCEEDED( buffer2.CreateAlias( buffer1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, *renderContext ) );
		EXPECT_TRUE( buffer2.IsValid() );
	}
	else
	{
		Tr2GpuBufferAL buffer1;
		ASSERT_HRESULT_FAILED( buffer1.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		Tr2GpuBufferAL buffer2;
		ASSERT_HRESULT_FAILED( buffer2.CreateAlias( buffer1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, *renderContext ) );
		EXPECT_FALSE( buffer2.IsValid() );
	}
}

TEST_F( WithValidRenderContext, GpuBufferIsInvalidAfterDestruction )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		vb.Destroy();
		EXPECT_FALSE( vb.IsValid() );
	}
}

TEST_F( WithValidRenderContext, GpuBufferReportsCorrectSizeAndType )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
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
		ASSERT_HRESULT_SUCCEEDED( vb.CreateStructured( 128, 32, 0, nullptr, *renderContext ) );
		EXPECT_TRUE( vb.IsValid() );
		EXPECT_EQ( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN, vb.GetFormat() );
	}
}

TEST_F( WithValidRenderContext, GpuBufferEqualsItself )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb;
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		EXPECT_TRUE( vb == vb );
	}
}

TEST_F( WithValidRenderContext, DifferentGpuBuffersAreNotEqual )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb1;
		ASSERT_HRESULT_SUCCEEDED( vb1.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		Tr2GpuBufferAL vb2;
		ASSERT_HRESULT_SUCCEEDED( vb2.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
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
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_READ, nullptr, *renderContext ) );
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
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, *renderContext ) );
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
		ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2RenderContextEnum::USAGE_CPU_READ, nullptr, *renderContext ) );
		auto memoryClass = vb.GetMemoryClass();
		EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
	}
}

TEST_F( WithValidRenderContext, CanCopyGpuBuffer )
{
	if( renderContext->GetCaps().SupportsGpuBuffer() )
	{
		Tr2GpuBufferAL vb1;
		ASSERT_HRESULT_SUCCEEDED( vb1.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		Tr2GpuBufferAL vb2;
		ASSERT_HRESULT_SUCCEEDED( vb2.Create( 128, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( vb1.CopySubBuffer( 0, 4, vb2, 16, *renderContext ) );
	}
}
