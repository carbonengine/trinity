#pragma once
#ifndef Tr2TextureALDx9_h_
#define Tr2TextureALDx9_h_


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2BitmapDimensions.h"


struct Tr2SubresourceData;
class Tr2RenderContextAL;
struct Tr2TextureSubresource;



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

		ALResult Attach( IDirect3DSurface9* surface, Tr2PrimaryRenderContextAL& renderContext );
	private:
		ALResult GetSurfaceLevel( CComPtr<IDirect3DSurface9>& surface, uint32_t face, uint32_t mip, Tr2RenderContextAL& renderContext );
		ALResult Lock( const Tr2TextureSubresource& region, uint32_t lockType, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
		void Unlock();


		CComPtr<IDirect3DBaseTexture9> m_texture;
		CComPtr<IDirect3DTexture9> m_mipGeneratedRT;
		CComPtr<IDirect3DSurface9> m_surface;
		HANDLE m_sharedHandle;
		D3DPOOL m_pool;
		D3DFORMAT m_format;
		CComPtr<IDirect3DSurface9> m_lockedSurface;

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
