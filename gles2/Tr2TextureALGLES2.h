#pragma once

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )



#include "../include/Tr2TextureAL.h"
#include "../include/Tr2BitmapDimensions.h"
#include "Tr2SamplerStateALGLES2.h"
#include "../Tr2MemoryCounterAL.h"


struct Tr2SubresourceData;
class Tr2RenderContextAL;
struct Tr2TextureSubresource;




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
	private:

		CcpMallocBuffer m_lockedData;

		Tr2TextureSubresource m_lockedRegion;

		TrinityALImpl::Tr2SamplerStateAL::StateData m_currentSampler;

		GLuint m_texture;
		GLuint m_msaaBuffer;
		GLuint m_stencilBuffer;

		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2GpuUsage::Type m_gpuUsage;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2MemoryCounterAL m_memory;
#if TRINITY_AL_GUARD_LOCKS
		Tr2LockGuard m_lockGuard;
#endif

		friend class ::Tr2PrimaryRenderContextAL;
		friend class ::Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};
}

#endif 
