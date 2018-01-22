#pragma once
#ifndef Tr2TextureALGLES2_h_
#define Tr2TextureALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2BitmapDimensions.h"
#include "../Tr2HalHelperStructures.h"
#include "../Tr2MemoryCounterAL.h"
#include "../Tr2LockGuard.h"


struct Tr2SubresourceData;
class Tr2RenderContextAL;
struct Tr2TextureSubresource;


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

	private:

		CcpMallocBuffer m_lockedData;

		Tr2TextureSubresource m_lockedRegion;

		GLuint m_texture;
		GLuint m_msaaBuffer;
		mutable cl_mem m_clObject;

		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2GpuUsage::Type m_gpuUsage;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2MemoryCounterAL m_memory;
#if TRINITY_AL_GUARD_LOCKS
		Tr2LockGuard m_lockGuard;
#endif

		friend class Tr2PrimaryRenderContextAL;
		friend class Tr2RenderContextAL;
		friend class Tr2ResourceSetAL;
	};
}

#endif

#endif
