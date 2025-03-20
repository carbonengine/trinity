////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2BufferAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../Tr2LockGuard.h"
#include "Tr2ResourceHelper.h"
#include "./util/DescriptorHeapViewDx12.h"

namespace TrinityALImpl
{
	class Tr2RtShaderTableAL;

	class Tr2BufferAL : public Tr2DeviceResourceAL<Tr2BufferAL>
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

		Tr2ALMemoryType GetMemoryClass() const;
		const Tr2BufferDescriptionAL& GetDesc() const;

		ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForReading( Tr2RenderContextAL& renderContext );

		ALResult MapForWriting( void*& data, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );

		ID3D12Resource* GetGpuResource();
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuView();

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );
		D3D12_RESOURCE_STATES GetDefaultState() const;

		uint32_t GetSrvIndexInHeap() const;
		uint32_t GetUavIndexInHeap() const;

	private:
		Tr2BufferDescriptionAL m_desc;
		Tr2ResourceHelper m_buffer;

		Tr2PrimaryRenderContextAL* m_owner;
		D3D12_RESOURCE_STATES m_defaultState;
		CComPtr<ID3D12Resource> m_lockedScratch;

		D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc;
		D3D12_UNORDERED_ACCESS_VIEW_DESC m_uavDesc;
		std::shared_ptr<ShaderResourceViewDx12> m_srv;
		std::shared_ptr<UnorderedAccessViewDx12> m_uav;
		std::shared_ptr<UnorderedAccessViewDx12> m_clearUav;

		friend class Tr2RenderContextAL;
		friend class TrinityALImpl::Tr2ResourceSetAL;
		friend class TrinityALImpl::Tr2RtShaderTableAL;
	};
}

#endif