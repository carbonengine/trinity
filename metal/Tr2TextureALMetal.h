#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2TextureAL.h"
#include "../include/Tr2BitmapDimensions.h"
#include "../Tr2HalHelperStructures.h"
#include "MetalContext.h"

namespace TrinityALImpl
{
	class Tr2TextureAL : public Tr2DeviceResourceAL<Tr2TextureAL>
	{
	public:
		Tr2TextureAL();

		ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
		ALResult OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext );
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
		uintptr_t GetSharedHandle() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

		id<MTLTexture> GetMetalTexture() { return m_mtlTexture; };
		id<MTLTexture> GetSRGBViewMetalTexture();
		id<MTLTexture> GetUAVMetalTexture( uint32_t mipLevel );

		void AssignFromTexture( Tr2TextureAL& backBuffer );
	private:
		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2GpuUsage::Type m_gpuUsage;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2SubresourceData m_dataLayout;
		
		Tr2TextureSubresource m_mappedRegion;
        
        struct MappedRange
        {
            uint32_t offset;
            uint32_t size;
            uint64_t frame;
        };
        
        id<MTLBuffer> m_mtlWriteBuffer;
        std::vector<MappedRange> m_mappedRanges;
        MappedRange m_mappedRange;

		id<MTLBuffer>   m_mtlReadBackBuffer;
		id<MTLTexture>  m_mtlTexture;
		id<MTLTexture>  m_mtlTextureSRGBView;
		std::vector<id<MTLTexture>>  m_mtlTextureUAV;

		MetalContext   *m_metalContext;
		bool            m_wrappedTexture;
	};
}

#endif
