#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX9

#include "../include/Tr2BufferAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../Tr2LockGuard.h"

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
		Tr2ALMemoryType GetMemoryClass() const;
		const Tr2BufferDescriptionAL& GetDesc() const;

		ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForReading( Tr2RenderContextAL& renderContext );
		ALResult MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	private:
		ALResult Lock( void** data, uint32_t flags );
		void Unlock();

		CComPtr<IDirect3DVertexBuffer9> m_vertexBuffer;
		CComPtr<IDirect3DIndexBuffer9>	m_indexBuffer;
		D3DPOOL m_pool;
		Tr2MemoryCounterAL m_memory;
		Tr2BufferDescriptionAL m_desc;

		friend class Tr2RenderContextAL;
		friend class Tr2PrimaryRenderContextAL;
	};
}

#endif