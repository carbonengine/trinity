// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"

#include <atomic>
#include <chrono>
#include <thread>

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
extern bool g_asyncCreateUseCopyQueueDx12;
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
extern bool g_asyncCreateAllowConcurrentCreatesDx11;
#endif

using namespace Tr2RenderContextEnum;

struct AsyncCreation : public WithValidRenderContext
{
	// Recreates the device of the shared render context (for tests that destroy it or change creation settings)
	static void RecreateDevice()
	{
		renderContext->Destroy();
		ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) );
	}

	// Calls Present until predicate returns true (or a timeout expires)
	template <typename Predicate>
	static bool PresentUntil( Predicate predicate )
	{
		auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 30 );
		while( !predicate() )
		{
			if( std::chrono::steady_clock::now() > deadline )
			{
				return false;
			}
			if( FAILED( renderContext->Present() ) )
			{
				return false;
			}
		}
		return true;
	}
};

namespace
{

std::vector<float> MakeData( size_t count, float base )
{
	std::vector<float> data( count );
	for( size_t i = 0; i < count; ++i )
	{
		data[i] = base + float( i );
	}
	return data;
}

std::vector<uint32_t> MakePixels( uint32_t width, uint32_t height, uint32_t seed )
{
	std::vector<uint32_t> pixels( width * height );
	for( uint32_t i = 0; i < pixels.size(); ++i )
	{
		pixels[i] = ( seed * 2654435761u ) ^ ( i * 40503u );
	}
	return pixels;
}

void ExpectBufferContents( const Tr2BufferAL& buffer, const std::vector<float>& expected, Tr2RenderContextAL& renderContext )
{
	ASSERT_TRUE( buffer.IsValid() );
	const float* data = nullptr;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForReading( data, renderContext ) );
	for( size_t i = 0; i < expected.size(); ++i )
	{
		if( expected[i] != data[i] )
		{
			ADD_FAILURE() << "Buffer element " << i << " is " << data[i] << ", expected " << expected[i];
			break;
		}
	}
	buffer.UnmapForReading( renderContext );
}

void ExpectTextureContents( Tr2TextureAL texture, uint32_t mip, const std::vector<uint32_t>& expected, Tr2RenderContextAL& renderContext )
{
	ASSERT_TRUE( texture.IsValid() );
	uint32_t width = std::max( texture.GetWidth() >> mip, 1u );
	uint32_t height = std::max( texture.GetHeight() >> mip, 1u );
	ASSERT_EQ( size_t( width * height ), expected.size() );

	const void* data = nullptr;
	uint32_t pitch = 0;
	ASSERT_HRESULT_SUCCEEDED( texture.MapForReading( Tr2TextureSubresource( mip ), data, pitch, renderContext ) );
	for( uint32_t y = 0; y < height; ++y )
	{
		auto row = reinterpret_cast<const uint32_t*>( static_cast<const uint8_t*>( data ) + y * pitch );
		if( memcmp( row, &expected[y * width], width * sizeof( uint32_t ) ) != 0 )
		{
			ADD_FAILURE() << "Texture row " << y << " of mip " << mip << " doesn't match initial data";
			break;
		}
	}
	texture.UnmapForReading( renderContext );
}

Tr2SubresourceData SubresourceData( const std::vector<uint32_t>& pixels, uint32_t width )
{
	Tr2SubresourceData data;
	data.m_sysMem = pixels.data();
	data.m_sysMemPitch = width * sizeof( uint32_t );
	data.m_sysMemSlicePitch = uint32_t( pixels.size() * sizeof( uint32_t ) );
	return data;
}

}


TEST_F( AsyncCreation, DefaultFutureIsInvalid )
{
	Tr2FutureAL<Tr2BufferAL> future;
	EXPECT_FALSE( future.IsValid() );
	EXPECT_FALSE( future.IsReady() );
	EXPECT_HRESULT_FAILED( future.Wait() );
	EXPECT_HRESULT_FAILED( future.GetResult() );
	EXPECT_FALSE( future.Get().IsValid() );
}

TEST_F( AsyncCreation, CreatesBuffer )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 64, 1.0f );
	auto future = renderContext->CreateBufferAsync( sizeof( float ), uint32_t( data.size() ), Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	ASSERT_TRUE( future.IsValid() );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );
	EXPECT_TRUE( future.IsReady() );
	EXPECT_HRESULT_SUCCEEDED( future.GetResult() );

	auto buffer = future.Get();
	ASSERT_TRUE( buffer.IsValid() );
	EXPECT_EQ( data.size() * sizeof( float ), buffer.GetSize() );
	ExpectBufferContents( buffer, data, *renderContext );
}

TEST_F( AsyncCreation, CreatesBufferWithEachOverload )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 16, 10.0f );

	auto fromDesc = renderContext->CreateBufferAsync( Tr2BufferDescriptionAL( sizeof( float ), 16, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ ), data.data() );
	auto fromFormat = renderContext->CreateBufferAsync( PIXEL_FORMAT_R32_FLOAT, 16, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, data.data() );
	auto fromStride = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	auto writable = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, nullptr );
	auto dynamic = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, data.data() );

	ASSERT_HRESULT_SUCCEEDED( fromDesc.Wait() );
	ASSERT_HRESULT_SUCCEEDED( fromFormat.Wait() );
	ASSERT_HRESULT_SUCCEEDED( fromStride.Wait() );
	ASSERT_HRESULT_SUCCEEDED( writable.Wait() );
	ASSERT_HRESULT_SUCCEEDED( dynamic.Wait() );

	ExpectBufferContents( fromDesc.Get(), data, *renderContext );
	ExpectBufferContents( fromFormat.Get(), data, *renderContext );
	ExpectBufferContents( fromStride.Get(), data, *renderContext );
	EXPECT_EQ( PIXEL_FORMAT_R32_FLOAT, fromFormat.Get().GetDesc().format );

	auto buffer = writable.Get();
	ASSERT_TRUE( buffer.IsValid() );
	float* mapped = nullptr;
	ASSERT_HRESULT_SUCCEEDED( buffer.MapForWriting( mapped, *renderContext ) );
	buffer.UnmapForWriting( *renderContext );

	EXPECT_TRUE( dynamic.Get().IsValid() );
}

TEST_F( AsyncCreation, CreatesTexture )
{
	ENSURE_GPU_OR_SKIP
	auto pixels = MakePixels( 64, 32, 1 );
	Tr2SubresourceData initialData[] = { SubresourceData( pixels, 64 ) };

	auto future = renderContext->CreateTextureAsync( Tr2BitmapDimensions( 64, 32, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData );
	ASSERT_TRUE( future.IsValid() );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );

	auto texture = future.Get();
	ASSERT_TRUE( texture.IsValid() );
	EXPECT_EQ( 64u, texture.GetWidth() );
	EXPECT_EQ( 32u, texture.GetHeight() );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, texture.GetFormat() );
	ExpectTextureContents( texture, 0, pixels, *renderContext );
}

TEST_F( AsyncCreation, CreatesMipMappedTexture )
{
	ENSURE_GPU_OR_SKIP
	std::vector<std::vector<uint32_t>> mips;
	std::vector<Tr2SubresourceData> initialData;
	for( uint32_t mip = 0; mip < 4; ++mip )
	{
		uint32_t size = 32 >> mip;
		mips.push_back( MakePixels( size, size, mip + 7 ) );
	}
	for( uint32_t mip = 0; mip < 4; ++mip )
	{
		initialData.push_back( SubresourceData( mips[mip], 32 >> mip ) );
	}

	auto future = renderContext->CreateTextureAsync( Tr2BitmapDimensions( 32, 32, 4, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData.data() );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );
	auto texture = future.Get();
	ASSERT_TRUE( texture.IsValid() );
	EXPECT_EQ( 4u, texture.GetTrueMipCount() );
	for( uint32_t mip = 0; mip < 4; ++mip )
	{
		ExpectTextureContents( texture, mip, mips[mip], *renderContext );
	}
}

TEST_F( AsyncCreation, CreatesTextureWithEachOverload )
{
	ENSURE_GPU_OR_SKIP
	auto pixels = MakePixels( 16, 16, 3 );
	Tr2SubresourceData initialData[] = { SubresourceData( pixels, 16 ) };
	Tr2BitmapDimensions desc( 16, 16, 1, PIXEL_FORMAT_B8G8R8A8_UNORM );

	std::vector<Tr2FutureAL<Tr2TextureAL>> futures;
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2GpuUsage::RENDER_TARGET ) );
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2MsaaDesc( 1 ), Tr2GpuUsage::RENDER_TARGET ) );
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2GpuUsage::SHADER_RESOURCE, initialData ) );
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE ) );
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData ) );
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2MsaaDesc(), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData ) );
	futures.push_back( renderContext->CreateTextureAsync( Tr2BitmapDimensions( 16, 16, 1, PIXEL_FORMAT_D32_FLOAT ), Tr2GpuUsage::DEPTH_STENCIL ) );
	futures.push_back( renderContext->CreateTextureAsync( desc, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, initialData ) );

	for( auto& future : futures )
	{
		ASSERT_HRESULT_SUCCEEDED( future.Wait() );
		EXPECT_TRUE( future.Get().IsValid() );
	}
	ExpectTextureContents( futures[4].Get(), 0, pixels, *renderContext );
	ExpectTextureContents( futures[5].Get(), 0, pixels, *renderContext );
}

TEST_F( AsyncCreation, CreatedResourcesCanBeUsedByGpu )
{
	ENSURE_GPU_OR_SKIP
#if TRINITY_PLATFORM_SUPPORTS_COMPUTE
	uint8_t csBytecode[] = {
#include INCLUDE_SHADER_CODE( AddConstantToBuffer.cs )
	};

	auto signature = Tr2ShaderSignatureAL()
						 .Add( Tr2ShaderRegisterAL::UAV_BUFFER, 0 )
						 .Add( Tr2ShaderRegisterAL::SRV_BUFFER, 0 )
						 .Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 1 )
						 .Add( Tr2ShaderThreadGroupSizeAL( 1, 1, 1 ) );
	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, csBytecode, signature, "", *renderContext ) );
	Tr2ShaderAL shaders[] = { cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	float data1[] = { 1.f, 2.f, 3.f, 4.f };
	float data2[] = { 100.f, 200.f, 300.f, 400.f };

	auto input = renderContext->CreateBufferAsync( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, data1 );
	auto output = renderContext->CreateBufferAsync( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::READ, nullptr );

	Tr2ConstantBufferAL constants;
	ASSERT_HRESULT_SUCCEEDED( constants.Create( sizeof( data2 ), *renderContext ) );
	float* mapped;
	ASSERT_HRESULT_SUCCEEDED( constants.Lock( (void**)&mapped, *renderContext ) );
	memcpy( mapped, data2, sizeof( data2 ) );
	ASSERT_HRESULT_SUCCEEDED( constants.Unlock( *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( input.Wait() );
	ASSERT_HRESULT_SUCCEEDED( output.Wait() );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetSrv( COMPUTE_SHADER, 0, input.Get() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetUav( COMPUTE_SHADER, 0, output.Get() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( constants, COMPUTE_SHADER, 1 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->ResetResourceBindings() );

	ExpectBufferContents( output.Get(), { 101.f, 202.f, 303.f, 404.f }, *renderContext );
#endif
}

TEST_F( AsyncCreation, ResultIsPendingUntilMainThreadDispatches )
{
	ENSURE_GPU_OR_SKIP
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	// DX12 creation is always finished on the main thread
	auto data = MakeData( 4, 0.0f );
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	EXPECT_FALSE( future.IsReady() );
	EXPECT_EQ( E_PENDING, future.GetResult().GetResult() );
	EXPECT_FALSE( future.Get().IsValid() );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );
	EXPECT_TRUE( future.IsReady() );
#endif
}

TEST_F( AsyncCreation, ThenIsCalledOnMainThreadDuringPresent )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 4, 0.0f );
	bool called = false;
	Tr2BufferAL received;
	std::thread::id calledOn;

	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	future.Then( [&]( const Tr2BufferAL& buffer ) {
		called = true;
		received = buffer;
		calledOn = std::this_thread::get_id();
	} );

	// Callbacks only run from Present or Wait, never directly from the creation call or Then
	EXPECT_FALSE( called );
	ASSERT_TRUE( PresentUntil( [&] { return called; } ) );
	EXPECT_EQ( std::this_thread::get_id(), calledOn );
	EXPECT_TRUE( received == future.Get() );
	ExpectBufferContents( received, data, *renderContext );
}

TEST_F( AsyncCreation, ThenIsCalledDuringWaitOnMainThread )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 4, 0.0f );
	int calls = 0;

	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() );
	future.Then( [&]( const Tr2BufferAL& buffer ) {
		EXPECT_TRUE( buffer.IsValid() );
		++calls;
	} );
	future.Then( [&]( const Tr2BufferAL& ) { ++calls; } );
	future.Catch( [&]( HRESULT ) { ADD_FAILURE() << "Catch called for successful creation"; } );

	ASSERT_HRESULT_SUCCEEDED( future.Wait() );
	EXPECT_EQ( 2, calls );

	// Callbacks are only called once
	ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );
	EXPECT_EQ( 2, calls );
}

TEST_F( AsyncCreation, ThenAttachedAfterCompletionIsCalledOnNextDispatch )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 4, 0.0f );
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );

	bool called = false;
	future.Then( [&]( const Tr2BufferAL& buffer ) {
		EXPECT_TRUE( buffer.IsValid() );
		called = true;
	} );
	EXPECT_FALSE( called );
	ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
	EXPECT_TRUE( called );
}

TEST_F( AsyncCreation, CatchIsCalledWhenCreationFails )
{
	ENSURE_GPU_OR_SKIP
	bool thenCalled = false;
	HRESULT caught = S_OK;
	std::thread::id calledOn;

	// Zero-sized buffer is invalid
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 0, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, nullptr );
	future
		.Then( [&]( const Tr2BufferAL& ) { thenCalled = true; } )
		.Catch( [&]( HRESULT hr ) {
			caught = hr;
			calledOn = std::this_thread::get_id();
		} );
	EXPECT_EQ( S_OK, caught );

	EXPECT_EQ( E_INVALIDARG, future.Wait().GetResult() );
	EXPECT_TRUE( future.IsReady() );
	EXPECT_EQ( E_INVALIDARG, future.GetResult().GetResult() );
	EXPECT_FALSE( future.Get().IsValid() );
	EXPECT_EQ( E_INVALIDARG, caught );
	EXPECT_EQ( std::this_thread::get_id(), calledOn );
	EXPECT_FALSE( thenCalled );
}

TEST_F( AsyncCreation, CatchAttachedAfterFailureIsCalledOnNextDispatch )
{
	ENSURE_GPU_OR_SKIP
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, nullptr );
	EXPECT_HRESULT_FAILED( future.Wait() );

	HRESULT caught = S_OK;
	future.Catch( [&]( HRESULT hr ) { caught = hr; } );
	EXPECT_EQ( S_OK, caught );
	ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
	EXPECT_EQ( future.GetResult().GetResult(), caught );
	EXPECT_HRESULT_FAILED( caught );
}

TEST_F( AsyncCreation, ImmutableBufferWithoutInitialDataFails )
{
	ENSURE_GPU_OR_SKIP
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, nullptr );
	EXPECT_HRESULT_FAILED( future.Wait() );
	EXPECT_FALSE( future.Get().IsValid() );
}

TEST_F( AsyncCreation, InvalidTextureFails )
{
	ENSURE_GPU_OR_SKIP
	auto zeroSize = renderContext->CreateTextureAsync( Tr2BitmapDimensions( 0, 16, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE );
	auto immutableWithoutData = renderContext->CreateTextureAsync( Tr2BitmapDimensions( 16, 16, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE );

	HRESULT caught = S_OK;
	zeroSize.Catch( [&]( HRESULT hr ) { caught = hr; } );

	EXPECT_HRESULT_FAILED( zeroSize.Wait() );
	EXPECT_HRESULT_FAILED( immutableWithoutData.Wait() );
	EXPECT_FALSE( zeroSize.Get().IsValid() );
	EXPECT_FALSE( immutableWithoutData.Get().IsValid() );
	EXPECT_HRESULT_FAILED( caught );
}

TEST_F( AsyncCreation, FailuresDontAffectOtherCreations )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 8, 5.0f );
	std::vector<Tr2FutureAL<Tr2BufferAL>> good;
	std::vector<Tr2FutureAL<Tr2BufferAL>> bad;
	for( int i = 0; i < 10; ++i )
	{
		good.push_back( renderContext->CreateBufferAsync( sizeof( float ), 8, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() ) );
		bad.push_back( renderContext->CreateBufferAsync( sizeof( float ), 0, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() ) );
	}
	for( int i = 0; i < 10; ++i )
	{
		ASSERT_HRESULT_SUCCEEDED( good[i].Wait() );
		ExpectBufferContents( good[i].Get(), data, *renderContext );
		EXPECT_HRESULT_FAILED( bad[i].Wait() );
	}
}

TEST_F( AsyncCreation, CreatesManyResources )
{
	ENSURE_GPU_OR_SKIP
	const int count = 200;
	std::vector<std::vector<float>> data;
	std::vector<Tr2FutureAL<Tr2BufferAL>> futures;
	for( int i = 0; i < count; ++i )
	{
		data.push_back( MakeData( 32, float( i * 100 ) ) );
	}
	for( int i = 0; i < count; ++i )
	{
		futures.push_back( renderContext->CreateBufferAsync( sizeof( float ), 32, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, data[i].data() ) );
	}
	for( int i = 0; i < count; ++i )
	{
		ASSERT_HRESULT_SUCCEEDED( futures[i].Wait() );
		ExpectBufferContents( futures[i].Get(), data[i], *renderContext );
	}
}

TEST_F( AsyncCreation, CreatesResourcesFromMultipleThreads )
{
	ENSURE_GPU_OR_SKIP
	const int threadCount = 8;
	const int perThread = 16;

	std::vector<std::vector<float>> data( threadCount * perThread );
	std::vector<std::vector<uint32_t>> pixels( threadCount * perThread );
	for( int i = 0; i < threadCount * perThread; ++i )
	{
		data[i] = MakeData( 64, float( i * 1000 ) );
		pixels[i] = MakePixels( 32, 32, i );
	}

	std::vector<Tr2FutureAL<Tr2BufferAL>> buffers( threadCount * perThread );
	std::vector<Tr2FutureAL<Tr2TextureAL>> textures( threadCount * perThread );
	std::vector<Tr2SubresourceData> textureData( threadCount * perThread );
	std::atomic<int> callbacksOnWrongThread( 0 );
	std::atomic<int> callbackCount( 0 );
	std::atomic<int> waitFailures( 0 );
	std::atomic<int> finishedThreads( 0 );
	auto mainThread = std::this_thread::get_id();

	std::vector<std::thread> threads;
	for( int t = 0; t < threadCount; ++t )
	{
		threads.emplace_back( [&, t]() {
			for( int i = t * perThread; i < ( t + 1 ) * perThread; ++i )
			{
				buffers[i] = renderContext->CreateBufferAsync( sizeof( float ), 64, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data[i].data() );
				textureData[i] = SubresourceData( pixels[i], 32 );
				textures[i] = renderContext->CreateTextureAsync( Tr2BitmapDimensions( 32, 32, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, &textureData[i] );
				buffers[i].Then( [&]( const Tr2BufferAL& ) {
					++callbackCount;
					if( std::this_thread::get_id() != mainThread )
					{
						++callbacksOnWrongThread;
					}
				} );
			}
			// Waiting on a worker thread blocks until the main thread finishes the creations
			for( int i = t * perThread; i < ( t + 1 ) * perThread; ++i )
			{
				if( FAILED( buffers[i].Wait() ) || FAILED( textures[i].Wait() ) )
				{
					++waitFailures;
				}
			}
			++finishedThreads;
		} );
	}

	bool finished = PresentUntil( [&] { return finishedThreads == threadCount; } );
	for( auto& thread : threads )
	{
		thread.join();
	}
	ASSERT_TRUE( finished );
	EXPECT_EQ( 0, waitFailures.load() );

	ASSERT_TRUE( PresentUntil( [&] { return callbackCount == threadCount * perThread; } ) );
	EXPECT_EQ( 0, callbacksOnWrongThread.load() );

	for( int i = 0; i < threadCount * perThread; ++i )
	{
		ExpectBufferContents( buffers[i].Get(), data[i], *renderContext );
		ExpectTextureContents( textures[i].Get(), 0, pixels[i], *renderContext );
	}
}

TEST_F( AsyncCreation, MainThreadCanWaitForFuturesCreatedOnOtherThreads )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 16, 3.0f );
	std::vector<Tr2FutureAL<Tr2BufferAL>> futures( 4 );
	std::vector<std::thread> threads;
	for( size_t i = 0; i < futures.size(); ++i )
	{
		threads.emplace_back( [&, i]() {
			futures[i] = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
		} );
	}
	for( auto& thread : threads )
	{
		thread.join();
	}
	for( auto& future : futures )
	{
		ASSERT_HRESULT_SUCCEEDED( future.Wait() );
		ExpectBufferContents( future.Get(), data, *renderContext );
	}
}

TEST_F( AsyncCreation, CallbacksCanCreateAndWaitForResources )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 4, 0.0f );
	Tr2BufferAL nested;
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() );
	future.Then( [&]( const Tr2BufferAL& ) {
		// Waiting from a callback dispatches recursively
		auto inner = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
		EXPECT_HRESULT_SUCCEEDED( inner.Wait() );
		nested = inner.Get();
	} );
	ASSERT_HRESULT_SUCCEEDED( future.Wait() );
	ExpectBufferContents( nested, data, *renderContext );
}

TEST_F( AsyncCreation, DroppingFutureDoesNotCancelCreation )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 4, 0.0f );
	Tr2BufferAL received;
	renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() ).Then( [&]( const Tr2BufferAL& buffer ) {
		received = buffer;
	} );
	// Futures that nobody holds still complete
	for( int i = 0; i < 10; ++i )
	{
		renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() );
	}
	ASSERT_TRUE( PresentUntil( [&] { return received.IsValid(); } ) );
	ExpectBufferContents( received, data, *renderContext );
}

TEST_F( AsyncCreation, DestroyingContextCancelsCreationsInFlight )
{
	ENSURE_GPU_OR_SKIP
	const int count = 64;
	auto pixels = MakePixels( 512, 512, 11 );
	auto data = MakeData( 1024, 0.0f );
	Tr2SubresourceData initialData[] = { SubresourceData( pixels, 512 ) };

	std::vector<Tr2FutureAL<Tr2TextureAL>> textures;
	std::vector<Tr2FutureAL<Tr2BufferAL>> buffers;
	int thenCount = 0;
	int catchCount = 0;
	int abortCount = 0;
	for( int i = 0; i < count; ++i )
	{
		textures.push_back( renderContext->CreateTextureAsync( Tr2BitmapDimensions( 512, 512, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, initialData ) );
		textures.back().Then( [&]( const Tr2TextureAL& ) { ++thenCount; } ).Catch( [&]( HRESULT hr ) {
			++catchCount;
			abortCount += hr == E_ABORT ? 1 : 0;
		} );
		buffers.push_back( renderContext->CreateBufferAsync( sizeof( float ), 1024, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() ) );
		buffers.back().Then( [&]( const Tr2BufferAL& ) { ++thenCount; } ).Catch( [&]( HRESULT hr ) {
			++catchCount;
			abortCount += hr == E_ABORT ? 1 : 0;
		} );
	}

	renderContext->Destroy();

	// All creations have completed, and all callbacks ran during Destroy
	EXPECT_EQ( 2 * count, thenCount + catchCount );
	EXPECT_EQ( catchCount, abortCount );
	for( int i = 0; i < count; ++i )
	{
		EXPECT_TRUE( textures[i].IsReady() );
		EXPECT_TRUE( buffers[i].IsReady() );
		auto textureResult = textures[i].GetResult().GetResult();
		auto bufferResult = buffers[i].GetResult().GetResult();
		EXPECT_TRUE( textureResult == S_OK || textureResult == E_ABORT );
		EXPECT_TRUE( bufferResult == S_OK || bufferResult == E_ABORT );
		EXPECT_EQ( textureResult == S_OK, textures[i].Get().IsValid() );
	}
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	// Nothing was finished on the main thread before Destroy
	EXPECT_EQ( 2 * count, abortCount );
#endif

	textures.clear();
	buffers.clear();
	RecreateDevice();
}

TEST_F( AsyncCreation, CreatingWithInvalidContextFails )
{
	ENSURE_GPU_OR_SKIP
	renderContext->Destroy();

	auto data = MakeData( 4, 0.0f );
	auto future = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() );
	EXPECT_TRUE( future.IsReady() );
	HRESULT caught = S_OK;
	future.Catch( [&]( HRESULT hr ) { caught = hr; } );
	EXPECT_HRESULT_FAILED( future.Wait() );
	EXPECT_HRESULT_FAILED( caught );
	EXPECT_FALSE( future.Get().IsValid() );

	RecreateDevice();

	// Creation works again with the new device
	auto recreated = renderContext->CreateBufferAsync( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	ASSERT_HRESULT_SUCCEEDED( recreated.Wait() );
	ExpectBufferContents( recreated.Get(), data, *renderContext );
}

TEST_F( AsyncCreation, DeletingContextCancelsCreationsInFlight )
{
	ENSURE_GPU_OR_SKIP
	auto pixels = MakePixels( 256, 256, 5 );
	Tr2SubresourceData initialData[] = { SubresourceData( pixels, 256 ) };
	std::vector<Tr2FutureAL<Tr2TextureAL>> futures;
	int callbacks = 0;
	for( int i = 0; i < 32; ++i )
	{
		futures.push_back( renderContext->CreateTextureAsync( Tr2BitmapDimensions( 256, 256, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, initialData ) );
		futures.back().Then( [&]( const Tr2TextureAL& ) { ++callbacks; } ).Catch( [&]( HRESULT ) { ++callbacks; } );
	}

	delete renderContext;
	renderContext = new Tr2PrimaryRenderContextAL();
	Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( renderContext );

	EXPECT_EQ( 32, callbacks );
	for( auto& future : futures )
	{
		EXPECT_TRUE( future.IsReady() );
		auto result = future.GetResult().GetResult();
		EXPECT_TRUE( result == S_OK || result == E_ABORT );
	}
	futures.clear();

	ASSERT_HRESULT_SUCCEEDED( renderContext->CreateDevice( 0, WithWindow::GetWindowHandle(), presentParameters ) );
}

TEST_F( AsyncCreation, DestroyingContextWhileOtherThreadsCreateResources )
{
	ENSURE_GPU_OR_SKIP
	auto data = MakeData( 256, 0.0f );
	std::atomic<bool> stop( false );
	std::atomic<int> submitted( 0 );
	// Callbacks run on the main thread
	int created = 0;
	int cancelled = 0;
	int unexpected = 0;

	// Like with synchronous creation, resources must not outlive the render context, so the threads
	// don't keep the futures; created resources are only referenced by callbacks that run on the main thread.
	std::vector<std::thread> threads;
	for( int t = 0; t < 4; ++t )
	{
		threads.emplace_back( [&]() {
			while( !stop )
			{
				renderContext->CreateBufferAsync( sizeof( float ), 256, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data.data() )
					.Then( [&]( const Tr2BufferAL& buffer ) {
						created += buffer.IsValid() ? 1 : 0;
					} )
					.Catch( [&]( HRESULT hr ) {
						if( hr == E_ABORT || hr == E_INVALIDCALL )
						{
							++cancelled;
						}
						else
						{
							++unexpected;
						}
					} );
				++submitted;
				std::this_thread::yield();
			}
		} );
	}

	// Let the threads create some resources and then pull the device from under them
	bool createdSome = PresentUntil( [&] { return created > 50; } );
	renderContext->Destroy();
	auto submittedAtDestroy = submitted.load();
	// Threads keep creating with the destroyed context; their futures fail immediately
	while( submitted < submittedAtDestroy + 100 )
	{
		std::this_thread::yield();
	}
	stop = true;
	for( auto& thread : threads )
	{
		thread.join();
	}

	RecreateDevice();
	// Run callbacks posted after Destroy while the counters are still alive
	ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );

	EXPECT_TRUE( createdSome );
	EXPECT_GE( cancelled, 100 );
	EXPECT_EQ( submitted.load(), created + cancelled + unexpected );
	EXPECT_EQ( 0, unexpected );
}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
TEST_F( AsyncCreation, CreatesResourcesWithoutCopyQueue )
{
	ENSURE_GPU_OR_SKIP
	g_asyncCreateUseCopyQueueDx12 = false;
	RecreateDevice();

	auto data = MakeData( 64, 1.0f );
	auto pixels = MakePixels( 64, 64, 2 );
	Tr2SubresourceData initialData[] = { SubresourceData( pixels, 64 ) };

	std::vector<Tr2FutureAL<Tr2BufferAL>> buffers;
	std::vector<Tr2FutureAL<Tr2TextureAL>> textures;
	std::vector<std::thread> threads;
	std::mutex mutex;
	for( int t = 0; t < 4; ++t )
	{
		threads.emplace_back( [&]() {
			for( int i = 0; i < 8; ++i )
			{
				auto buffer = renderContext->CreateBufferAsync( sizeof( float ), 64, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, data.data() );
				auto texture = renderContext->CreateTextureAsync( Tr2BitmapDimensions( 64, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData );
				std::lock_guard<std::mutex> lock( mutex );
				buffers.push_back( buffer );
				textures.push_back( texture );
			}
		} );
	}
	for( auto& thread : threads )
	{
		thread.join();
	}
	for( size_t i = 0; i < buffers.size(); ++i )
	{
		ASSERT_HRESULT_SUCCEEDED( buffers[i].Wait() );
		ASSERT_HRESULT_SUCCEEDED( textures[i].Wait() );
		ExpectBufferContents( buffers[i].Get(), data, *renderContext );
		ExpectTextureContents( textures[i].Get(), 0, pixels, *renderContext );
	}

	// Resources must be released before the device is destroyed
	buffers.clear();
	textures.clear();
	g_asyncCreateUseCopyQueueDx12 = true;
	RecreateDevice();
}
#endif

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
TEST_F( AsyncCreation, CreationsFromOtherThreadsAreQueuedWithoutConcurrentCreates )
{
	ENSURE_GPU_OR_SKIP
	g_asyncCreateAllowConcurrentCreatesDx11 = false;
	RecreateDevice();

	auto data = MakeData( 16, 2.0f );

	// Main thread creations are synchronous
	auto onMain = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	EXPECT_TRUE( onMain.IsReady() );

	// Creations from other threads wait for the main thread
	Tr2FutureAL<Tr2BufferAL> onWorker;
	std::thread worker( [&]() {
		onWorker = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	} );
	worker.join();
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	EXPECT_FALSE( onWorker.IsReady() );
	ASSERT_HRESULT_SUCCEEDED( onWorker.Wait() );
	ExpectBufferContents( onWorker.Get(), data, *renderContext );

	// Worker thread waiting for a queued creation is released by Present on the main thread
	std::atomic<bool> done( false );
	HRESULT workerResult = E_FAIL;
	std::thread waiter( [&]() {
		auto future = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
		workerResult = future.Wait().GetResult();
		done = true;
	} );
	bool finished = PresentUntil( [&] { return done.load(); } );
	waiter.join();
	EXPECT_TRUE( finished );
	EXPECT_HRESULT_SUCCEEDED( workerResult );

	// Queued creations are cancelled when the context is destroyed (resources must be released before that)
	onMain = Tr2FutureAL<Tr2BufferAL>();
	onWorker = Tr2FutureAL<Tr2BufferAL>();
	Tr2FutureAL<Tr2BufferAL> queued;
	std::thread queuer( [&]() {
		queued = renderContext->CreateBufferAsync( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::READ, data.data() );
	} );
	queuer.join();
	HRESULT caught = S_OK;
	queued.Catch( [&]( HRESULT hr ) { caught = hr; } );
	renderContext->Destroy();
	EXPECT_EQ( E_ABORT, queued.GetResult().GetResult() );
	EXPECT_EQ( E_ABORT, caught );

	g_asyncCreateAllowConcurrentCreatesDx11 = true;
	RecreateDevice();
}
#endif
