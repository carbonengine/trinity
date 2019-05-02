////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2MemoryCounterAL.h"

#ifdef TRINITY_AL_GUARD_LOCKS
#include "../Tr2LockGuard.h"
#endif


class Tr2PrimaryRenderContextAL;
struct Tr2TextureSubresource;
class Tr2RenderContextAL;
struct Tr2SubresourceData;
struct Tr2BitmapDimensions;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}



namespace TrinityALImpl
{
	class Tr2TextureAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TEXTURE>
	{
	public:
		Tr2TextureAL();
		~Tr2TextureAL();

		ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
		ALResult OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void Destroy();

		bool IsValid() const;

		Tr2ALMemoryType GetMemoryClass() const;
		const Tr2BitmapDimensions& GetDesc() const;
		const Tr2MsaaDesc& GetMsaaDesc() const;
		Tr2GpuUsage::Type GetGpuUsage() const;
		Tr2CpuUsage::Type GetCpuUsage() const;

		ALResult MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
		void UnmapForReading( Tr2RenderContextAL& renderContext );
		ALResult MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
		void UnmapForWriting( Tr2RenderContextAL& renderContext );

		ALResult UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext );
		ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext );
		ALResult GenerateMipMaps( Tr2RenderContextAL& renderContext );
		ALResult Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext );

		uintptr_t GetSharedHandle() const
		{
			return 0;
		}

		ID3D12Resource* GetResourceDx12() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDescriptorHandleDx12( Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR ) const;
		void AssignFromSwapChainDx12( const std::vector<CComPtr<ID3D12Resource>>& backBuffers, const CComPtr<ID3D12DescriptorHeap>& descriptors, uint32_t descriptorSize, Tr2PrimaryRenderContextAL& renderContext );
		void SetSwapChainBufferIndexDx12( uint32_t index );
	private:
		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2GpuUsage::Type m_gpuUsage;

		std::vector<CComPtr<ID3D12Resource>> m_textures;
		CComPtr<ID3D12DescriptorHeap> m_rtvDescriptors;
		CComPtr<ID3D12DescriptorHeap> m_dsDescriptors;

		CComPtr<ID3D12Resource> m_writeScratch;
		CComPtr<ID3D12Resource> m_readScratch;
		Tr2TextureSubresource m_mappedRegion;

		Tr2PrimaryRenderContextAL* m_owner;
		uint32_t m_rtvDescriptorSize;
		uint32_t m_srvDescriptorSize;
		uint32_t m_currentTextureIndex;
		D3D12_RESOURCE_STATES m_defaultState;

		D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc[2];
		std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> m_uavDesc;

		// TODO: this does not belong here
		CComPtr<ID3D12DescriptorHeap> m_uavDescriptors;

		friend class Tr2ResourceSetAL;
		friend class Tr2RenderContextAL;
	};
}

#endif
