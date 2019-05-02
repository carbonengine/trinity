////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2BufferAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../Tr2LockGuard.h"

namespace TrinityALImpl
{
	class Tr2BufferAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_BUFFER>
	{
	public:
		ALResult Create(
			const Tr2BufferDescriptionAL& desc,
			const void* initialData,
			Tr2PrimaryRenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void Destroy()
		{

		}

		bool IsValid() const
		{
			return false;
		}

		Tr2ALMemoryType GetMemoryClass()
		{
			return AL_MEMORY_VIDEO;
		}

		const Tr2BufferDescriptionAL& GetDesc() const
		{
			return m_desc;
		}

		ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void UnmapForReading( Tr2RenderContextAL& renderContext )
		{
		}
		ALResult MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void UnmapForWriting( Tr2RenderContextAL& renderContext )
		{
		}

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
		{
			return E_NOTIMPL;;
		}
	private:
		Tr2MemoryCounterAL m_memory;
		Tr2BufferDescriptionAL m_desc;
	};
}

#endif