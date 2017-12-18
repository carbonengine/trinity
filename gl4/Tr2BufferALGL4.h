#pragma once

#if TRINITY_PLATFORM == TRINITY_OPENGL4

#include "../include/Tr2BufferAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../Tr2LockGuard.h"

namespace TrinityALImpl
{
	class Tr2BufferAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_BUFFER>
	{
	public:
		Tr2BufferAL();
		~Tr2BufferAL();

		ALResult Create(
			const Tr2BufferDescriptionAL& desc,
			const void* initialData,
			Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass();
		const Tr2BufferDescriptionAL& GetDesc() const;

		ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForReading( Tr2RenderContextAL& renderContext );
		ALResult MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );
	private:
		GLuint m_buffer;
		GLuint m_textureBuffer;
		mutable cl_mem m_clObject;
		Tr2MemoryCounterAL m_memory;
		Tr2BufferDescriptionAL m_desc;

		friend class ::Tr2RenderContextAL;
		friend class ::Tr2PrimaryRenderContextAL;
		friend class Tr2ResourceSetAL;
	};
}

#endif