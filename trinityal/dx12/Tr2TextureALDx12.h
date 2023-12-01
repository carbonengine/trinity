////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


#include "../include/Tr2TextureAL.h"
#include "../Tr2MemoryCounterAL.h"

#ifdef TRINITY_AL_GUARD_LOCKS
#include "../Tr2LockGuard.h"
#endif

#include "./util/DescriptorHeapViewDx12.h"


namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}



namespace TrinityALImpl
{
	class Tr2TextureAL : public Tr2DeviceResourceAL<Tr2TextureAL>
	{
	public:
		Tr2TextureAL();
		~Tr2TextureAL();

		ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
		ALResult OpenShared( uintptr_t, Tr2GpuUsage::Type, Tr2PrimaryRenderContextAL& )
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
		const std::shared_ptr<RenderTargetViewDx12>& GetRtvDescriptorHandleDx12( Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR, uint32_t slice = 0 ) const;
		void AssignFromSwapChainDx12( const std::vector<CComPtr<ID3D12Resource>>& backBuffers, const std::vector<std::shared_ptr<RenderTargetViewDx12>>& rtvs, Tr2PrimaryRenderContextAL& renderContext );
		void SetSwapChainBufferIndexDx12( uint32_t index );

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

		bool operator==( const Tr2TextureAL& other ) const;
	private:
		void GetRegionSize( const Tr2TextureSubresource& region, uint32_t& pitch, uint64_t& size );

		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2GpuUsage::Type m_gpuUsage;

		std::vector<CComPtr<ID3D12Resource>> m_textures;

		struct WriteScratch
		{
			CComPtr<ID3D12Resource> scratch;
			uint64_t size;
			uint64_t frameIndex;
		};
		typedef std::vector<WriteScratch> WriteScratches;

		WriteScratches m_writeScratches;
		WriteScratches::iterator m_mappedScratch;

		CComPtr<ID3D12Resource> m_readScratch;
		Tr2TextureSubresource m_mappedRegion;

		Tr2PrimaryRenderContextAL* m_owner;
		uint32_t m_currentTextureIndex;
		D3D12_RESOURCE_STATES m_defaultState;

		D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc[2];
		std::vector<std::shared_ptr<UnorderedAccessViewDx12>> m_uav;
		std::shared_ptr<ShaderResourceViewDx12> m_view[2];
		std::vector<std::shared_ptr<RenderTargetViewDx12>> m_rtv;
		std::shared_ptr<DepthStencilViewDx12> m_dsv;
		std::string m_name;

		struct MipMapGenerator;
		std::unique_ptr<MipMapGenerator> m_mipMapGenerator;

		friend class Tr2ResourceSetAL;
		friend class Tr2RenderContextAL;
	};
}

#endif
