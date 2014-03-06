#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2BufferImplALOrbis.h"

using namespace Tr2RenderContextEnum;


Tr2BufferImplAL::Tr2BufferImplAL()
	:m_size( 0 ),
	m_align( 0 ),
	m_currentBuffer( 0 ),
	m_bufferType( STATIC ),
	m_cpuAccess( READ_ONLY ),
	m_gpuAccess( READ_ONLY ),
	m_memoryType( Tr2MemoryAllocator::GARLIC )
{
}

Tr2BufferImplAL::~Tr2BufferImplAL()
{
	Destroy();
}

Tr2BufferImplAL& Tr2BufferImplAL::operator=( Tr2BufferImplAL&& other )
{
	m_size = other.m_size;
	m_align = other.m_align;
	m_bufferData = std::move( other.m_bufferData );
	m_currentBuffer = other.m_currentBuffer;
	m_bufferType = other.m_bufferType;
	m_cpuAccess = other.m_cpuAccess;
	m_gpuAccess = other.m_gpuAccess;
	m_memoryType = other.m_memoryType;

	other.m_bufferData.clear();
	other.m_size = 0;

	return *this;
}

ALResult Tr2BufferImplAL::Create( 
	uint32_t size,
	uint32_t align,
	Access cpuAccess,
	Access gpuAccess,
	BufferType bufferType, 
	Tr2MemoryAllocator::MemoryType memoryType,
	void** initialData, 
	Tr2PrimaryRenderContextAL &renderContext )
{
	BufferData newData;
	newData.frameUsed = renderContext.InternalGetCurentFrameIndex() + renderContext.InternalGetMaxFrameLatency() + 1;
	newData.data = Tr2MemoryAllocator::Allocate( memoryType, size, align );
	if( !newData.data )
	{
		return E_OUTOFMEMORY;
	}
	m_bufferData.push_back( newData );

	m_size = size;
	m_align = align;
	m_bufferType = bufferType;
	m_currentBuffer = 0;
	m_memoryType = memoryType;

	if( initialData )
	{
		*initialData = newData.data;
	}

	return S_OK;
}

void Tr2BufferImplAL::Destroy()
{
	for( auto it = m_bufferData.begin(); it != m_bufferData.end(); ++it )
	{
		Tr2RenderContextAL::InternalDelayDelete( it->frameUsed, it->data );
	}
	m_bufferData.clear();
	m_size = 0;
}

bool Tr2BufferImplAL::IsValid() const
{
	return !m_bufferData.empty();
}

uint32_t Tr2BufferImplAL::GetSize() const
{
	return m_size;
}

void* Tr2BufferImplAL::GetMemoryForCpuWriting( Tr2RenderContextAL & renderContext )
{
	// Find any buffer that is guaranteed to be unused by GPU at the moment
	uint32_t currentFrame = renderContext.InternalGetCurentFrameIndex();
	uint32_t maxDistance = renderContext.InternalGetMaxFrameLatency();
	for( uint32_t i = 0; i < m_bufferData.size(); ++i )
	{
		uint32_t distance = std::min( currentFrame - m_bufferData[i].frameUsed, m_bufferData[i].frameUsed - currentFrame );
		if( distance > maxDistance )
		{
			m_currentBuffer = i;
			return m_bufferData[i].data;
		}
	}
	if( m_bufferType == DYNAMIC )
	{
		// For dynamic buffer we just create a new one
		BufferData newData;
		newData.frameUsed = currentFrame;
		newData.data = Tr2MemoryAllocator::Allocate( m_memoryType, m_size, m_align );
		m_bufferData.push_back( newData );
		m_currentBuffer = m_bufferData.size() - 1;
		return newData.data;
	}
	else
	{
		// For "static" buffers we create one additional (to a total of 2) or sync to GPU
		if( m_bufferData.size() < 2 )
		{
			BufferData newData;
			newData.frameUsed = currentFrame;
			newData.data = Tr2MemoryAllocator::Allocate( m_memoryType, m_size, m_align );
			m_bufferData.push_back( newData );
			m_currentBuffer = m_bufferData.size() - 1;
			return newData.data;
		}
		renderContext.InternalSyncToGpu();
		m_currentBuffer = 0;
		return m_bufferData[0].data;
	}
}

void* Tr2BufferImplAL::GetMemoryForCpuReading( Tr2RenderContextAL & renderContext ) const
{
	if( m_gpuAccess == READ_WRITE )
	{
		renderContext.InternalSyncToGpu();
	}
	return m_bufferData[m_currentBuffer].data;
}

void* Tr2BufferImplAL::GetMemoryForGpuReading( Tr2RenderContextAL & renderContext ) const
{
	m_bufferData[m_currentBuffer].frameUsed = renderContext.InternalGetCurentFrameIndex();
	return m_bufferData[m_currentBuffer].data;
}

void* Tr2BufferImplAL::GetMemoryForCpuWritingNoSync( Tr2RenderContextAL & renderContext )
{
	return m_bufferData[m_currentBuffer].data;
}

#endif