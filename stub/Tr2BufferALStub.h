#pragma once

#if TRINITY_PLATFORM == TRINITY_STUB

#include "../include/Tr2BufferAL.h"

namespace TrinityALImpl
{
	class Tr2BufferAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_BUFFER>
	{
	public:
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
		ALResult MapForWriting( void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );
	private:
		CcpMallocBuffer m_buffer;
		Tr2BufferDescriptionAL m_desc;

		friend class Tr2RenderContextAL;
		friend class Tr2PrimaryRenderContextAL;
	};
}

#endif