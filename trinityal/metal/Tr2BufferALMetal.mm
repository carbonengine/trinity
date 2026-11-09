// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2BufferALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "MetalContext.h"
#include "Tr2AsyncCreatorMetal.h"

namespace
{
template <typename T> bool HasFlag( T value, T flag )
{
	return ( value & flag ) == flag;
}

ALResult CheckCreationArguments( const Tr2BufferDescriptionAL& desc, const void* initialData, Tr2PrimaryRenderContextAL& renderContext )
{
	if( desc.count == 0 )
	{
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	bool isImmutable =
		!HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) && !HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS );
	if( isImmutable && !initialData )
	{
		return E_INVALIDARG;
	}

	if( HasFlag( desc.cpuUsage, Tr2CpuUsage::READ ) && HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
	{
		return E_INVALIDARG;
	}
	return S_OK;
}

uint32_t GetStride( const Tr2BufferDescriptionAL& desc )
{
	if( desc.format != Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
	{
		return GetBytesPerPixel( desc.format );
	}
	return desc.stride;
}

MTLResourceOptions GetResourceMode( const Tr2BufferDescriptionAL& desc )
{
	if( !HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) &&
		!HasFlag( desc.cpuUsage, Tr2CpuUsage::NON_SYNCRONIZED_WRITE ) )
	{
		// No point keeping a copy on the CPU side if they'll never be modified.
		return MTLResourceStorageModePrivate;
	}
	return MTLResourceStorageModeManaged;
}
}

namespace TrinityALImpl
{
Tr2BufferAL::Tr2BufferAL() :
	m_owner( nullptr ), m_metalContext( nullptr ), m_heapIndex( 0xffffffff ), m_usedInEncoder( 0 ), m_asyncBuffer( nil )
{
}

Tr2BufferAL::~Tr2BufferAL()
{
	Destroy();
}

ALResult Tr2BufferAL::Create( const Tr2BufferDescriptionAL& desc,
							  const void* initialData,
							  Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	FORWARD_HR( CheckCreationArguments( desc, initialData, renderContext ) );

	auto stride = GetStride( desc );

	m_metalContext = renderContext.GetMetalContext();
	auto bufferSizeInBytes = desc.count * stride;

	m_resourceMode = GetResourceMode( desc );

	m_owner = &renderContext;
	m_mtlBuffer = m_metalContext->CreateMetalBuffer(
		renderContext.GetMetalWorkQueue(), bufferSizeInBytes, m_resourceMode, initialData );
	m_desc = desc;
	if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) ||
		HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
	{
		m_heapIndex = m_metalContext->AllocateHeapIndex( m_mtlBuffer );
	}
	m_memory.Set( Tr2MemoryCounterAL::BUFFER, bufferSizeInBytes );

	return m_mtlBuffer ? S_OK : E_FAIL;
}

ALResult Tr2BufferAL::CreateAsyncMetal( const Tr2BufferDescriptionAL& desc,
										const void* initialData,
										Tr2PrimaryRenderContextAL& renderContext,
										Tr2AsyncUploadMetal& upload )
{
	FORWARD_HR( CheckCreationArguments( desc, initialData, renderContext ) );

	auto bufferSizeInBytes = desc.count * GetStride( desc );
	auto resourceMode = GetResourceMode( desc );
	id<MTLDevice> device = upload.GetDevice();

	id<MTLBuffer> buffer = nil;
	if( initialData && ( resourceMode & MTLResourceStorageModeMask ) == MTLResourceStorageModePrivate )
	{
		buffer = [device newBufferWithLength:bufferSizeInBytes options:resourceMode];
		id<MTLBuffer> staging = upload.CreateStagingBuffer( initialData, bufferSizeInBytes );
		if( !buffer || !staging )
		{
			return E_OUTOFMEMORY;
		}
		[upload.GetBlitEncoder() copyFromBuffer:staging sourceOffset:0 toBuffer:buffer destinationOffset:0 size:bufferSizeInBytes];
	}
	else if( initialData )
	{
		buffer = [device newBufferWithBytes:initialData length:bufferSizeInBytes options:resourceMode];
	}
	else
	{
		buffer = [device newBufferWithLength:bufferSizeInBytes options:resourceMode];
	}
	if( !buffer )
	{
		return E_OUTOFMEMORY;
	}

	m_asyncBuffer = buffer;
	m_desc = desc;
	m_resourceMode = resourceMode;
	return S_OK;
}

ALResult Tr2BufferAL::FinishCreateAsyncMetal( Tr2PrimaryRenderContextAL& renderContext )
{
	if( !m_asyncBuffer )
	{
		return E_INVALIDCALL;
	}
	m_metalContext = renderContext.GetMetalContext();
	m_owner = &renderContext;
	m_mtlBuffer = m_asyncBuffer;
	m_asyncBuffer = nil;
	if( HasFlag( m_desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) ||
		HasFlag( m_desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
	{
		m_heapIndex = m_metalContext->AllocateHeapIndex( m_mtlBuffer );
	}
	m_memory.Set( Tr2MemoryCounterAL::BUFFER, m_desc.count * GetStride( m_desc ) );
	return S_OK;
}

void Tr2BufferAL::Destroy()
{
	if( m_metalContext )
	{
		m_metalContext->DeallocateHeapIndex( m_heapIndex );
		m_metalContext->DestroyMetalBuffer( m_mtlBuffer );
		m_metalContext->DestroyMetalBuffer( m_mappedBuffer );
		for( auto& staging : m_stagingBuffers )
		{
			m_metalContext->DestroyMetalBuffer( staging.buffer );
		}
	}
	m_stagingBuffers.clear();
	m_mtlBuffer = nil;
	m_mappedBuffer = nil;
	m_asyncBuffer = nil;
	m_metalContext = nil;
	m_desc.count = 0;
	m_heapIndex = 0xffffffff;
	m_owner = nullptr;
	m_memory.Reset();
	m_usedInEncoder = 0;
}

bool Tr2BufferAL::IsValid() const
{
	return m_mtlBuffer != nil;
}

ALResult Tr2BufferAL::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !IsValid() )
	{
		data = nullptr;
		return E_INVALIDCALL;
	}
	return MapForReading( data, 0, uint32_t( m_mtlBuffer.length ), renderContext );
}

ALResult
	Tr2BufferAL::MapForReading( const void*& data, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !IsValid() )
	{
		data = nullptr;
		return E_INVALIDCALL;
	}
	if( size == 0 || offset + size > m_desc.stride * m_desc.count )
	{
		data = nullptr;
		return E_INVALIDARG;
	}

	if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::READ ) )
	{
		return E_INVALIDCALL;
	}

	MetalContext* metalContext = renderContext.GetMetalContext();

	m_mappedBuffer =
		metalContext->CreateMetalBuffer( renderContext.GetMetalWorkQueue(), size, MTLResourceStorageModeManaged, nil );
	renderContext.GetMetalWorkQueue()->CopyBufferToBuffer( m_mappedBuffer, 0, m_mtlBuffer, offset, size );
	renderContext.GetMetalWorkQueue()->ReadBackBufferToCPU( m_mappedBuffer, true );

	m_memory.Grow( m_mappedBuffer.length );

	data = m_mappedBuffer.contents;
	return S_OK;
}

void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& renderContext )
{
	if( m_mappedBuffer )
	{
		m_memory.Shrink( m_mappedBuffer.length );
		MetalContext* metalContext = renderContext.GetMetalContext();
		metalContext->DestroyMetalBuffer( m_mappedBuffer );
		m_mappedBuffer = nil;
	}
}

ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !IsValid() )
	{
		data = nullptr;
		return E_INVALIDCALL;
	}
	if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
	{
		return E_INVALIDCALL;
	}

	if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::NON_SYNCRONIZED_WRITE ) )
	{
		if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			m_mappedBuffer = m_mtlBuffer;
			for( auto& staging : m_stagingBuffers )
			{
				if( staging.buffer == m_mappedBuffer )
				{
					staging.mappedFrameNumber = m_metalContext->GetRecordingFrameNumber();
					break;
				}
			}
			data = m_mappedBuffer.contents;
			return S_OK;
		}
		else
		{
			m_mappedBuffer = m_mtlBuffer;
			data = m_mappedBuffer.contents;
			return S_OK;
		}
	}

	if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
	{
		for( auto& staging : m_stagingBuffers )
		{
			if( !m_metalContext->IsResourceInUse( staging.mappedFrameNumber ) )
			{
				staging.mappedFrameNumber = m_metalContext->GetRecordingFrameNumber();
				m_mappedBuffer = staging.buffer;
				data = m_mappedBuffer.contents;
				return S_OK;
			}
		}
		m_mappedBuffer = m_metalContext->CreateMetalBuffer(
			renderContext.GetMetalWorkQueue(), m_mtlBuffer.length, MTLResourceStorageModeManaged, nil );
		if( !m_name.empty() )
		{
			m_mappedBuffer.label = [NSString stringWithUTF8String:m_name.c_str()];
		}
		StagingBuffer staging = { m_mappedBuffer, m_metalContext->GetRecordingFrameNumber() };
		m_stagingBuffers.push_back( staging );
		m_memory.Grow( m_mtlBuffer.length );
	}
	else
	{
		m_mappedBuffer = m_metalContext->CreateMetalBuffer(
			renderContext.GetMetalWorkQueue(), m_mtlBuffer.length, MTLResourceStorageModeManaged, nil );
		m_memory.Grow( m_mtlBuffer.length );
	}
	data = m_mappedBuffer.contents;
	return S_OK;
}

void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
{
	if( !m_mappedBuffer )
	{
		return;
	}
	m_metalContext->IndicateBufferModified( m_mappedBuffer, 0, m_mtlBuffer.length );
	if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::NON_SYNCRONIZED_WRITE ) )
	{
		m_mappedBuffer = nil;
	}
	else if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
	{
		if( HasFlag( m_desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) ||
			HasFlag( m_desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			renderContext.GetMetalWorkQueue()->CopyBufferToBuffer(
				m_mtlBuffer, 0, m_mappedBuffer, 0, m_mtlBuffer.length );
			m_mappedBuffer = nil;
		}
		else
		{
			m_owner->BufferRewritten( m_mtlBuffer, m_mappedBuffer );
			m_mtlBuffer = m_mappedBuffer;
		}
	}
	else
	{
		m_memory.Shrink( m_mappedBuffer.length );
		renderContext.GetMetalWorkQueue()->CopyBufferToBuffer( m_mtlBuffer, 0, m_mappedBuffer, 0, m_mtlBuffer.length );
		m_metalContext->DestroyMetalBuffer( m_mappedBuffer );
		m_mappedBuffer = nil;
	}
}

ALResult
	Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( offset + size > m_desc.count * m_desc.stride )
	{
		return E_INVALIDARG;
	}
	if( size == 0 )
	{
		return S_OK;
	}

	if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) ||
		HasFlag( m_desc.cpuUsage, Tr2CpuUsage::NON_SYNCRONIZED_WRITE ) )
	{
		void* dest;
		CR_RETURN_HR( MapForWriting( dest, renderContext ) );
		uint8_t* dst = static_cast<uint8_t*>( dest ) + offset;
		const uint8_t* src = static_cast<const uint8_t*>( data );
		memcpy( dst, src, size );
		UnmapForWriting( renderContext );
	}
	else if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
	{
		id<MTLBuffer> staging = m_metalContext->CreateMetalBuffer(
			renderContext.GetMetalWorkQueue(), size, MTLResourceStorageModeManaged, data );
		renderContext.GetMetalWorkQueue()->CopyBufferToBuffer( m_mtlBuffer, offset, staging, 0, size );
		m_metalContext->DestroyMetalBuffer( staging );
	}
	else
	{
		return E_INVALIDCALL;
	}


	return S_OK;
}

uint32_t Tr2BufferAL::GetSrvIndexInHeap() const
{
	return m_heapIndex;
}

uint32_t Tr2BufferAL::GetUavIndexInHeap() const
{
	return m_heapIndex;
}

void Tr2BufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2BufferAL";
	description["size"] = std::to_string( GetDesc().count * GetDesc().stride );
	description["cpuUsage"] = std::to_string( int( GetDesc().cpuUsage ) );
	description["gpuUsage"] = std::to_string( int( GetDesc().gpuUsage ) );
	description["format"] = std::to_string( int( GetDesc().format ) );
	description["stride"] = std::to_string( GetDesc().stride );
	description["count"] = std::to_string( GetDesc().count );
	description["name"] = m_name;
}

ALResult Tr2BufferAL::SetName( const char* name )
{
	m_name = name;
	if( m_mtlBuffer )
	{
		m_mtlBuffer.label = [NSString stringWithUTF8String:name];
	}
	for( auto& staging : m_stagingBuffers )
	{
		staging.buffer.label = [NSString stringWithUTF8String:name];
	}
	return S_OK;
}
}

#endif
