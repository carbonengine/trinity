#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../include/Tr2BufferAL.h"
#include "MetalContext.h"
#include "../Tr2MemoryCounterAL.h"

namespace TrinityALImpl
{
	class Tr2BufferAL : public Tr2DeviceResourceAL<Tr2BufferAL>
	{
	public:
		ALResult Create(
			const Tr2BufferDescriptionAL& desc,
			const void* initialData,
			Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
		const Tr2BufferDescriptionAL& GetDesc() const { return m_desc; }

		ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForReading( Tr2RenderContextAL& renderContext );
		ALResult MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

		id<MTLBuffer> GetMetalBuffer() { return m_mtlBuffer; }

	private:
		Tr2BufferDescriptionAL m_desc;
		Tr2RenderContextAL* m_owner;
		MetalContext *m_metalContext;
		MTLResourceOptions m_resourceMode;
		id<MTLBuffer> m_mtlBuffer;
        Tr2MemoryCounterAL m_memory;

        struct StagingBuffer
        {
            id<MTLBuffer> buffer;
            uint64_t mappedFrameNumber;
        };
        std::vector<StagingBuffer> m_stagingBuffers;
        id<MTLBuffer> m_mappedBuffer;
		std::string m_name;

		friend class Tr2RenderContextAL;
		friend class Tr2PrimaryRenderContextAL;
	};
}

#endif
