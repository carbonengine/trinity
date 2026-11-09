// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2PrimaryRenderContextAL.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2TextureAL.h"
#include "../ALLog.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2AsyncCreator )


namespace
{
template <typename T>
Tr2FutureAL<T> MakeRejectedFuture( const std::shared_ptr<TrinityALImpl::Tr2AsyncDispatcherAL>& dispatcher, HRESULT result )
{
	auto state = std::make_shared<TrinityALImpl::Tr2FutureStateAL<T>>( dispatcher );
	state->Reject( result );
	return Tr2FutureAL<T>( state );
}
}


void Tr2PrimaryRenderContextAL::StartAsyncCreation()
{
	m_asyncDispatcher->SetMainThread();

	auto creator = std::make_shared<TrinityALImpl::Tr2AsyncCreatorAL>( *this, m_asyncDispatcher );
	if( FAILED( creator->Initialize() ) )
	{
		CCP_AL_LOGERR( "Failed to initialize asynchronous resource creation" );
		creator->Shutdown();
		return;
	}
	std::weak_ptr<TrinityALImpl::Tr2AsyncCreatorAL> weakCreator = creator;
	m_asyncDispatcher->SetPump( [weakCreator]() {
		if( auto creator = weakCreator.lock() )
		{
			creator->ProcessCompleted();
		}
	} );

	std::lock_guard<std::mutex> lock( m_asyncCreatorMutex );
	m_asyncCreator = std::move( creator );
}

void Tr2PrimaryRenderContextAL::StopAsyncCreation()
{
	std::shared_ptr<TrinityALImpl::Tr2AsyncCreatorAL> creator;
	{
		std::lock_guard<std::mutex> lock( m_asyncCreatorMutex );
		creator.swap( m_asyncCreator );
	}
	m_asyncDispatcher->SetPump( nullptr );
	if( creator )
	{
		creator->Shutdown();
	}
	// Run Catch callbacks of the creations that have just been cancelled
	if( m_asyncDispatcher->IsMainThread() )
	{
		m_asyncDispatcher->Dispatch();
	}
}

void Tr2PrimaryRenderContextAL::DispatchAsyncCallbacks()
{
	m_asyncDispatcher->Dispatch();
}

std::shared_ptr<TrinityALImpl::Tr2AsyncCreatorAL> Tr2PrimaryRenderContextAL::GetAsyncCreator()
{
	std::lock_guard<std::mutex> lock( m_asyncCreatorMutex );
	return m_asyncCreator;
}

Tr2FutureAL<Tr2BufferAL> Tr2PrimaryRenderContextAL::CreateBufferAsync(
	const Tr2BufferDescriptionAL& desc,
	const void* initialData )
{
	auto creator = GetAsyncCreator();
	if( !creator )
	{
		return MakeRejectedFuture<Tr2BufferAL>( m_asyncDispatcher, E_INVALIDCALL );
	}
	return creator->CreateBuffer( desc, initialData, []( std::shared_ptr<TrinityALImpl::Tr2BufferAL> object ) {
		Tr2BufferAL buffer;
		buffer.m_buffer = std::move( object );
		return buffer;
	} );
}

Tr2FutureAL<Tr2BufferAL> Tr2PrimaryRenderContextAL::CreateBufferAsync(
	Tr2RenderContextEnum::PixelFormat format,
	uint32_t count,
	Tr2GpuUsage::Type gpuUsage,
	Tr2CpuUsage::Type cpuUsage,
	const void* initialData )
{
	return CreateBufferAsync( Tr2BufferDescriptionAL( format, count, gpuUsage, cpuUsage ), initialData );
}

Tr2FutureAL<Tr2BufferAL> Tr2PrimaryRenderContextAL::CreateBufferAsync(
	uint32_t stride,
	uint32_t count,
	Tr2GpuUsage::Type gpuUsage,
	Tr2CpuUsage::Type cpuUsage,
	const void* initialData )
{
	return CreateBufferAsync( Tr2BufferDescriptionAL( stride, count, gpuUsage, cpuUsage ), initialData );
}

Tr2FutureAL<Tr2TextureAL> Tr2PrimaryRenderContextAL::CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage )
{
	return CreateTextureAsync( desc, Tr2MsaaDesc(), gpuUsage, Tr2CpuUsage::NONE, nullptr );
}

Tr2FutureAL<Tr2TextureAL> Tr2PrimaryRenderContextAL::CreateTextureAsync( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage )
{
	return CreateTextureAsync( desc, msaa, gpuUsage, Tr2CpuUsage::NONE, nullptr );
}

Tr2FutureAL<Tr2TextureAL> Tr2PrimaryRenderContextAL::CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2SubresourceData* initialData )
{
	return CreateTextureAsync( desc, Tr2MsaaDesc(), gpuUsage, Tr2CpuUsage::NONE, initialData );
}

Tr2FutureAL<Tr2TextureAL> Tr2PrimaryRenderContextAL::CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage )
{
	return CreateTextureAsync( desc, Tr2MsaaDesc(), gpuUsage, cpuUsage, nullptr );
}

Tr2FutureAL<Tr2TextureAL> Tr2PrimaryRenderContextAL::CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData )
{
	return CreateTextureAsync( desc, Tr2MsaaDesc(), gpuUsage, cpuUsage, initialData );
}

Tr2FutureAL<Tr2TextureAL> Tr2PrimaryRenderContextAL::CreateTextureAsync( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData )
{
	auto creator = GetAsyncCreator();
	if( !creator )
	{
		return MakeRejectedFuture<Tr2TextureAL>( m_asyncDispatcher, E_INVALIDCALL );
	}
	return creator->CreateTexture( desc, msaa, gpuUsage, cpuUsage, initialData, []( std::shared_ptr<TrinityALImpl::Tr2TextureAL> object ) {
		Tr2TextureAL texture;
		texture.m_texture = std::move( object );
		return texture;
	} );
}
