#pragma once
#ifndef Tr2BufferImplALOrbis_H
#define Tr2BufferImplALOrbis_H

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2MemoryAllocator.h"

class Tr2BufferImplAL
{
public:
	enum BufferType
	{
		STATIC,
		DYNAMIC,
	};
	enum Access
	{
		READ_ONLY,
		READ_WRITE,
	};

    Tr2BufferImplAL();
	~Tr2BufferImplAL();

	Tr2BufferImplAL& operator=( Tr2BufferImplAL&& other );

	ALResult Create( 
		uint32_t size, 
		uint32_t align,
		Access cpuAccess,
		Access gpuAccess,
		BufferType bufferType, 
		Tr2MemoryAllocator::MemoryType memoryType,
		void** initialData, 
		Tr2PrimaryRenderContextAL &renderContext );
	void Destroy();
	bool IsValid() const;
	uint32_t GetSize() const;

	void* GetMemoryForCpuWriting( Tr2RenderContextAL & renderContext );
	void* GetMemoryForCpuWritingNoSync( Tr2RenderContextAL & renderContext );
	void* GetMemoryForCpuReading( Tr2RenderContextAL & renderContext ) const;
	void* GetMemoryForGpuReading( Tr2RenderContextAL & renderContext ) const;
private:
	struct BufferData
	{
		void* data;
		mutable uint32_t frameUsed;
	};
	uint32_t m_size;
	uint32_t m_align;
	std::vector<BufferData> m_bufferData;
	uint32_t m_currentBuffer;
	BufferType m_bufferType;
	Access m_cpuAccess;
	Access m_gpuAccess;
	Tr2MemoryAllocator::MemoryType m_memoryType;

	Tr2BufferImplAL( const Tr2BufferImplAL& ) /* = delete */;
};
/*
CPU / GPU		RO						RW
RO			  1 buffer, no sync			SyncToGpu, read
RW			  
*/

#endif

#endif