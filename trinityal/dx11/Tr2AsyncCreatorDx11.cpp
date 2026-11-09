// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2AsyncCreatorDx11.h"
#include "Tr2BufferALDx11.h"
#include "Tr2TextureALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"

bool g_asyncCreateAllowConcurrentCreatesDx11 = true;

namespace TrinityALImpl
{

Tr2AsyncCreatorAL::Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher ) :
	m_renderContext( renderContext ),
	m_dispatcher( std::move( dispatcher ) ),
	m_concurrentCreates( false )
{
}

Tr2AsyncCreatorAL::~Tr2AsyncCreatorAL()
{
	Shutdown();
}

ALResult Tr2AsyncCreatorAL::Initialize()
{
	if( !m_renderContext.m_d3dDevice11 )
	{
		return E_INVALIDCALL;
	}
	D3D11_FEATURE_DATA_THREADING threading = {};
	if( SUCCEEDED( m_renderContext.m_d3dDevice11->CheckFeatureSupport( D3D11_FEATURE_THREADING, &threading, sizeof( threading ) ) ) )
	{
		m_concurrentCreates = threading.DriverConcurrentCreates && g_asyncCreateAllowConcurrentCreatesDx11;
	}
	return S_OK;
}

void Tr2AsyncCreatorAL::Shutdown()
{
	m_guard.Shutdown();

	std::vector<std::shared_ptr<Tr2AsyncContextJobAL<Empty>>> queue;
	{
		std::lock_guard<std::mutex> lock( m_queueMutex );
		queue.swap( m_queue );
	}
	for( auto& job : queue )
	{
		job->Cancel( E_ABORT );
	}
}

void Tr2AsyncCreatorAL::ProcessCompleted()
{
	std::vector<std::shared_ptr<Tr2AsyncContextJobAL<Empty>>> queue;
	{
		std::lock_guard<std::mutex> lock( m_queueMutex );
		queue.swap( m_queue );
	}
	for( auto& job : queue )
	{
		Run( job );
	}
}

bool Tr2AsyncCreatorAL::SupportsConcurrentCreates() const
{
	return m_concurrentCreates;
}

void Tr2AsyncCreatorAL::Run( std::shared_ptr<Tr2AsyncContextJobAL<Empty>> job )
{
	if( m_concurrentCreates || m_dispatcher->IsMainThread() )
	{
		bool done = m_guard.Use( [&job]() {
			Empty context;
			job->Execute( context );
			job->Finish();
		} );
		if( !done )
		{
			job->Cancel( E_ABORT );
		}
		return;
	}

	{
		std::lock_guard<std::mutex> lock( m_queueMutex );
		// Shutdown takes the queue after shutting down the guard, so check it under the queue lock
		bool queued = m_guard.Use( [this, &job]() { m_queue.push_back( job ); } );
		if( queued )
		{
			job = nullptr;
		}
	}
	if( job )
	{
		job->Cancel( E_ABORT );
	}
	else
	{
		// Main thread may be waiting for the queued creation
		m_dispatcher->Notify();
	}
}

Tr2FutureAL<::Tr2BufferAL> Tr2AsyncCreatorAL::CreateBuffer(
	const Tr2BufferDescriptionAL& desc,
	const void* initialData,
	WrapBufferFunction wrap )
{
	auto state = std::make_shared<Tr2FutureStateAL<::Tr2BufferAL>>( m_dispatcher );
	auto& renderContext = m_renderContext;
	Run( std::make_shared<Tr2AsyncCreateJobAL<::Tr2BufferAL, Tr2BufferAL, Empty>>(
		state,
		[desc, initialData, &renderContext]( Tr2BufferAL& buffer, Empty& ) -> HRESULT {
			return buffer.Create( desc, initialData, renderContext ).GetResult();
		},
		nullptr,
		std::move( wrap ) ) );
	return Tr2FutureAL<::Tr2BufferAL>( state );
}

Tr2FutureAL<::Tr2TextureAL> Tr2AsyncCreatorAL::CreateTexture(
	const Tr2BitmapDimensions& desc,
	const Tr2MsaaDesc& msaa,
	Tr2GpuUsage::Type gpuUsage,
	Tr2CpuUsage::Type cpuUsage,
	Tr2SubresourceData* initialData,
	WrapTextureFunction wrap )
{
	auto state = std::make_shared<Tr2FutureStateAL<::Tr2TextureAL>>( m_dispatcher );
	auto& renderContext = m_renderContext;
	Run( std::make_shared<Tr2AsyncCreateJobAL<::Tr2TextureAL, Tr2TextureAL, Empty>>(
		state,
		[desc, msaa, gpuUsage, cpuUsage, initialData, &renderContext]( Tr2TextureAL& texture, Empty& ) -> HRESULT {
			return texture.Create( desc, msaa, gpuUsage, cpuUsage, initialData, renderContext ).GetResult();
		},
		nullptr,
		std::move( wrap ) ) );
	return Tr2FutureAL<::Tr2TextureAL>( state );
}

}

#endif
