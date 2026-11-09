// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2AsyncCreatorDx12.h"
#include "Tr2BufferALDx12.h"
#include "Tr2TextureALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"
#include "../ALLog.h"

bool g_asyncCreateUseCopyQueueDx12 = true;

namespace TrinityALImpl
{

Tr2AsyncCreatorAL::Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher ) :
	Tr2AsyncWorkerAL( dispatcher ),
	m_renderContext( renderContext ),
	m_dispatcher( std::move( dispatcher ) ),
	m_isCopyQueue( false ),
	m_fenceValue( 0 ),
	m_fenceEvent( nullptr )
{
}

Tr2AsyncCreatorAL::~Tr2AsyncCreatorAL()
{
	Shutdown();
	if( m_fenceEvent )
	{
		CloseHandle( m_fenceEvent );
	}
}

ALResult Tr2AsyncCreatorAL::Initialize()
{
	auto device = m_renderContext.m_device;
	if( !device )
	{
		return E_INVALIDCALL;
	}

	if( g_asyncCreateUseCopyQueueDx12 )
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		if( SUCCEEDED( device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_queue ) ) ) )
		{
			m_isCopyQueue = true;
			SetDebugName( m_queue, "Async creation copy queue" );
		}
		else
		{
			CCP_AL_LOGWARN( "Failed to create a copy queue for asynchronous resource creation; using the direct queue" );
		}
	}
	if( !m_queue )
	{
		// Command queues are free-threaded, so the worker can submit to the direct queue as long as
		// it uses its own command list and fence
		m_queue = m_renderContext.m_commandQueue;
		m_isCopyQueue = false;
	}

	auto listType = m_isCopyQueue ? D3D12_COMMAND_LIST_TYPE_COPY : D3D12_COMMAND_LIST_TYPE_DIRECT;
	CR_RETURN_HR( device->CreateCommandAllocator( listType, IID_PPV_ARGS( &m_commandAllocator ) ) );
	CR_RETURN_HR( device->CreateCommandList( 0, listType, m_commandAllocator, nullptr, IID_PPV_ARGS( &m_commandList ) ) );
	CR_RETURN_HR( m_commandList->Close() );
	SetDebugName( m_commandList, "Async creation command list" );

	CR_RETURN_HR( device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) );
	m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if( !m_fenceEvent )
	{
		return E_FAIL;
	}

	Start();
	return S_OK;
}

void Tr2AsyncCreatorAL::Shutdown()
{
	Stop();
}

void Tr2AsyncCreatorAL::ProcessCompleted()
{
	FinishCompleted();
}

bool Tr2AsyncCreatorAL::UsesCopyQueue() const
{
	return m_isCopyQueue;
}

Tr2FutureAL<::Tr2BufferAL> Tr2AsyncCreatorAL::CreateBuffer(
	const Tr2BufferDescriptionAL& desc,
	const void* initialData,
	WrapBufferFunction wrap )
{
	auto state = std::make_shared<Tr2FutureStateAL<::Tr2BufferAL>>( m_dispatcher );
	auto& renderContext = m_renderContext;
	Submit( std::make_shared<Tr2AsyncCreateJobAL<::Tr2BufferAL, Tr2BufferAL, Tr2AsyncUploadDx12>>(
		state,
		[desc, initialData, &renderContext]( Tr2BufferAL& buffer, Tr2AsyncUploadDx12& upload ) -> HRESULT {
			return buffer.CreateAsyncDx12( desc, initialData, renderContext, upload ).GetResult();
		},
		[&renderContext]( Tr2BufferAL& buffer ) -> HRESULT {
			return buffer.FinishCreateAsyncDx12( renderContext ).GetResult();
		},
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
	Submit( std::make_shared<Tr2AsyncCreateJobAL<::Tr2TextureAL, Tr2TextureAL, Tr2AsyncUploadDx12>>(
		state,
		[desc, msaa, gpuUsage, cpuUsage, initialData, &renderContext]( Tr2TextureAL& texture, Tr2AsyncUploadDx12& upload ) -> HRESULT {
			return texture.CreateAsyncDx12( desc, msaa, gpuUsage, cpuUsage, initialData, renderContext, upload ).GetResult();
		},
		[&renderContext]( Tr2TextureAL& texture ) -> HRESULT {
			return texture.FinishCreateAsyncDx12( renderContext ).GetResult();
		},
		std::move( wrap ) ) );
	return Tr2FutureAL<::Tr2TextureAL>( state );
}

void Tr2AsyncCreatorAL::ExecuteBatch( const std::vector<std::shared_ptr<Tr2AsyncJobAL>>& jobs )
{
	HRESULT hr = m_commandAllocator->Reset();
	if( SUCCEEDED( hr ) )
	{
		hr = m_commandList->Reset( m_commandAllocator, nullptr );
	}
	if( FAILED( hr ) )
	{
		for( auto& job : jobs )
		{
			static_cast<Tr2AsyncContextJobAL<Tr2AsyncUploadDx12>&>( *job ).SetExecuteError( hr );
		}
		return;
	}

	Tr2AsyncUploadDx12 upload;
	upload.commandList = m_commandList;
	upload.isCopyQueue = m_isCopyQueue;

	for( auto& job : jobs )
	{
		static_cast<Tr2AsyncContextJobAL<Tr2AsyncUploadDx12>&>( *job ).Execute( upload );
	}

	hr = m_commandList->Close();
	if( SUCCEEDED( hr ) )
	{
		ID3D12CommandList* const commandLists[] = { m_commandList };
		m_queue->ExecuteCommandLists( _countof( commandLists ), commandLists );
		hr = m_queue->Signal( m_fence, ++m_fenceValue );
	}
	if( SUCCEEDED( hr ) )
	{
		hr = WaitForUpload( m_fenceValue );
	}
	if( FAILED( hr ) )
	{
		CCP_AL_LOGERR( "Asynchronous resource upload failed (0x%08x)", uint32_t( hr ) );
		for( auto& job : jobs )
		{
			static_cast<Tr2AsyncContextJobAL<Tr2AsyncUploadDx12>&>( *job ).SetExecuteError( hr );
		}
	}

	// Upload heaps are released here once the GPU is done with them
	upload.scratches.clear();
}

HRESULT Tr2AsyncCreatorAL::WaitForUpload( uint64_t fenceValue )
{
	while( m_fence->GetCompletedValue() < fenceValue )
	{
		HRESULT hr = m_fence->SetEventOnCompletion( fenceValue, m_fenceEvent );
		if( FAILED( hr ) )
		{
			return hr;
		}
		WaitForSingleObject( m_fenceEvent, 100 );
	}
	// Completed value is UINT64_MAX when the device is removed
	return m_renderContext.m_device->GetDeviceRemovedReason();
}

}

#endif
