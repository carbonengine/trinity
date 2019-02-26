#pragma once
#ifndef Tr2TextureALDx11_h_
#define Tr2TextureALDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )


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

		ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
		ALResult OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass();
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
		uintptr_t GetSharedHandle() const;

		ALResult Attach( ID3D11Texture2D* texture, Tr2PrimaryRenderContextAL& renderContext );
	private:
		struct DepthOption
		{
			enum Type
			{
				READ_WRITE = 0,
				READ_ONLY = 1,

				COUNT = 2,
			};
		};

		ALResult CreateViews( 
			ID3D11Resource* texture, 
			const Tr2BitmapDimensions& desc, 
			const Tr2MsaaDesc& msaa, 
			Tr2GpuUsage::Type gpuUsage, 
			Tr2CpuUsage::Type cpuUsage, 
			bool createSrgb,
			Tr2PrimaryRenderContextAL& renderContext );

		CComPtr<ID3D11Resource> m_texture;
		CComPtr<ID3D11ShaderResourceView> m_view[Tr2RenderContextEnum::_COLOR_SPACE_COUNT];
		CComPtr<ID3D11RenderTargetView> m_renderTarget[Tr2RenderContextEnum::_COLOR_SPACE_COUNT];
		CComPtr<ID3D11DepthStencilView> m_depthStencil[DepthOption::COUNT];
		std::vector<CComPtr<ID3D11UnorderedAccessView>> m_uav;
		CComPtr<ID3D11Texture2D> m_stagingTexture;
		CcpAlignedMallocBuffer m_writeStaging;
		uint32_t m_writeLtrb[4];

		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2GpuUsage::Type m_gpuUsage;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2MemoryCounterAL m_memory;
#if TRINITY_AL_GUARD_LOCKS
		Tr2LockGuard m_lockGuard;
#endif
		uint32_t m_lockedSubresource;

		friend class Tr2PrimaryRenderContextAL;
		friend class Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};
}

#endif

#endif
