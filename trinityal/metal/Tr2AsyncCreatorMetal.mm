// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2AsyncCreatorMetal.h"
#include "Tr2BufferALMetal.h"
#include "Tr2TextureALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "../ALLog.h"

namespace TrinityALImpl
{

Tr2AsyncUploadMetal::Tr2AsyncUploadMetal( id<MTLDevice> device, id<MTLCommandBuffer> commandBuffer ) :
	m_device( device ),
	m_commandBuffer( commandBuffer ),
	m_blitEncoder( nil )
{
}

Tr2AsyncUploadMetal::~Tr2AsyncUploadMetal()
{
	EndEncoding();
}

id<MTLDevice> Tr2AsyncUploadMetal::GetDevice() const
{
	return m_device;
}

id<MTLBlitCommandEncoder> Tr2AsyncUploadMetal::GetBlitEncoder()
{
	if( !m_blitEncoder )
	{
		m_blitEncoder = [m_commandBuffer blitCommandEncoder];
	}
	return m_blitEncoder;
}

id<MTLBuffer> Tr2AsyncUploadMetal::CreateStagingBuffer( const void* data, size_t size )
{
	id<MTLBuffer> buffer = [m_device newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
	if( buffer )
	{
		m_stagingBuffers.push_back( buffer );
	}
	return buffer;
}

void Tr2AsyncUploadMetal::EndEncoding()
{
	if( m_blitEncoder )
	{
		[m_blitEncoder endEncoding];
		m_blitEncoder = nil;
	}
}

void Tr2AsyncUploadMetal::ReleaseStagingBuffers()
{
	m_stagingBuffers.clear();
}


Tr2AsyncCreatorAL::Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher ) :
	Tr2AsyncWorkerAL( dispatcher ),
	m_renderContext( renderContext ),
	m_dispatcher( std::move( dispatcher ) ),
	m_device( nil ),
	m_queue( nil )
{
}

Tr2AsyncCreatorAL::~Tr2AsyncCreatorAL()
{
	Shutdown();
}

ALResult Tr2AsyncCreatorAL::Initialize()
{
	auto metalContext = m_renderContext.GetMetalContext();
	if( !metalContext )
	{
		return E_INVALIDCALL;
	}
	m_device = metalContext->GetDevice();
	m_queue = [m_device newCommandQueue];
	if( !m_queue )
	{
		return E_FAIL;
	}
	m_queue.label = @"Async creation queue";
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

Tr2FutureAL<::Tr2BufferAL> Tr2AsyncCreatorAL::CreateBuffer(
	const Tr2BufferDescriptionAL& desc,
	const void* initialData,
	WrapBufferFunction wrap )
{
	auto state = std::make_shared<Tr2FutureStateAL<::Tr2BufferAL>>( m_dispatcher );
	auto& renderContext = m_renderContext;
	Submit( std::make_shared<Tr2AsyncCreateJobAL<::Tr2BufferAL, Tr2BufferAL, Tr2AsyncUploadMetal>>(
		state,
		[desc, initialData, &renderContext]( Tr2BufferAL& buffer, Tr2AsyncUploadMetal& upload ) -> HRESULT {
			return buffer.CreateAsyncMetal( desc, initialData, renderContext, upload ).GetResult();
		},
		[&renderContext]( Tr2BufferAL& buffer ) -> HRESULT {
			return buffer.FinishCreateAsyncMetal( renderContext ).GetResult();
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
	Submit( std::make_shared<Tr2AsyncCreateJobAL<::Tr2TextureAL, Tr2TextureAL, Tr2AsyncUploadMetal>>(
		state,
		[desc, msaa, gpuUsage, cpuUsage, initialData, &renderContext]( Tr2TextureAL& texture, Tr2AsyncUploadMetal& upload ) -> HRESULT {
			return texture.CreateAsyncMetal( desc, msaa, gpuUsage, cpuUsage, initialData, renderContext, upload ).GetResult();
		},
		[&renderContext]( Tr2TextureAL& texture ) -> HRESULT {
			return texture.FinishCreateAsyncMetal( renderContext ).GetResult();
		},
		std::move( wrap ) ) );
	return Tr2FutureAL<::Tr2TextureAL>( state );
}

void Tr2AsyncCreatorAL::ExecuteBatch( const std::vector<std::shared_ptr<Tr2AsyncJobAL>>& jobs )
{
	@autoreleasepool
	{
		id<MTLCommandBuffer> commandBuffer = [m_queue commandBuffer];
		commandBuffer.label = @"Async creation uploads";
		Tr2AsyncUploadMetal upload( m_device, commandBuffer );

		for( auto& job : jobs )
		{
			static_cast<Tr2AsyncContextJobAL<Tr2AsyncUploadMetal>&>( *job ).Execute( upload );
		}

		upload.EndEncoding();
		[commandBuffer commit];
		[commandBuffer waitUntilCompleted];

		if( commandBuffer.status == MTLCommandBufferStatusError )
		{
			CCP_AL_LOGERR( "Asynchronous resource upload failed" );
			for( auto& job : jobs )
			{
				static_cast<Tr2AsyncContextJobAL<Tr2AsyncUploadMetal>&>( *job ).SetExecuteError( E_FAIL );
			}
		}

		// Staging buffers are released here once the GPU is done with them
		upload.ReleaseStagingBuffers();
	}
}

}

#endif
