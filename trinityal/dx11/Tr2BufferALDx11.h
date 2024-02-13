#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

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
		ALResult MapForWriting( void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

		uint32_t GetSrvIndexInHeap() const;
		uint32_t GetUavIndexInHeap() const;

	private:
		ALResult CreateStagingBuffer( Tr2RenderContextAL& renderContext );

		CComPtr<ID3D11Buffer> m_buffer;
		CComPtr<ID3D11Buffer> m_staging;
		CComPtr<ID3D11ShaderResourceView> m_srv;
		CComPtr<ID3D11UnorderedAccessView> m_uav;
		Tr2MemoryCounterAL m_memory;
		Tr2BufferDescriptionAL m_desc;
		CcpAlignedMallocBuffer m_writeLockMemory;

#if TRINITY_AL_GUARD_LOCKS
		Tr2LockGuard m_lockGuard;
#endif
		std::string m_name;

		friend class Tr2RenderContextAL;
		friend class Tr2PrimaryRenderContextAL;
		friend class Tr2ResourceSetAL;
	};
}

#endif