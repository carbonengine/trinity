// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2TextureAL.h"
#include "../Tr2HalHelperStructures.h"
#include "../Tr2MemoryCounterAL.h"
#include "MetalContext.h"

namespace TrinityALImpl
{
class Tr2ReadbackAL : public Tr2DeviceResourceAL<Tr2ReadbackAL>
{
public:
	Tr2ReadbackAL( MetalContext* metalContext, id<MTLBuffer> mtlReadBackBuffer, uint32_t rowPitch, uint64_t frameNumber );

	~Tr2ReadbackAL();

	bool IsReady( Tr2PrimaryRenderContextAL& renderContext ) const;

	ALResult Map( const void*& pointer, uint32_t& rowPitch, Tr2PrimaryRenderContextAL& renderContext ) const;

	void Destroy();

	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	Tr2ALMemoryType GetMemoryClass() const;

	bool IsValid() const;

private:
	MetalContext* m_metalContext;
	id<MTLBuffer> m_mtlReadBackBuffer;
	uint32_t m_rowPitch;
	uint64_t m_frameNumber;
};

class Tr2AsyncUploadMetal;

class Tr2TextureAL : public Tr2DeviceResourceAL<Tr2TextureAL>
{
public:
	Tr2TextureAL();
	~Tr2TextureAL();

	ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );

	// Asynchronous creation (see Tr2AsyncCreatorMetal). CreateAsyncMetal runs on a worker thread: it creates
	// the texture and encodes the initial data upload into upload. FinishCreateAsyncMetal runs on the main
	// thread once the upload has completed.
	ALResult CreateAsyncMetal( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext, Tr2AsyncUploadMetal& upload );
	ALResult FinishCreateAsyncMetal( Tr2PrimaryRenderContextAL& renderContext );
	ALResult OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass() const;
	const Tr2BitmapDimensions& GetDesc() const;
	const Tr2MsaaDesc& GetMsaaDesc() const;
	Tr2GpuUsage::Type GetGpuUsage() const;
	Tr2CpuUsage::Type GetCpuUsage() const;

	std::shared_ptr<Tr2ReadbackAL> CreateReadback( const Tr2TextureSubresource& region, Tr2PrimaryRenderContextAL& renderContext );

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
	const char* GetName() const;
	ALResult SetName( const char* name );

	id<MTLTexture> GetMetalTexture()
	{
		return m_mtlTexture;
	};
	id<MTLTexture> GetSRGBViewMetalTexture();
	id<MTLTexture> GetUAVMetalTexture( uint32_t mipLevel );

	void AssignFromTexture( Tr2TextureAL& backBuffer );

	uint32_t GetSrvIndexInHeap( Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR ) const;
	uint32_t GetUavIndexInHeap( uint32_t mip ) const;

private:
	// Output of the resource creation phase
	struct CreatedResourceMetal
	{
		id<MTLTexture> texture = nil;
		Tr2BitmapDimensions realDesc;
	};

	// State kept between CreateAsyncMetal and FinishCreateAsyncMetal
	struct AsyncCreationMetal
	{
		Tr2BitmapDimensions desc;
		Tr2MsaaDesc msaa;
		Tr2GpuUsage::Type gpuUsage = Tr2GpuUsage::NONE;
		Tr2CpuUsage::Type cpuUsage = Tr2CpuUsage::NONE;
		bool hasDataLayout = false;
		Tr2SubresourceData dataLayout = { nullptr, 0, 0 };
		CreatedResourceMetal created;
	};

	ALResult CreateResource( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext, Tr2AsyncUploadMetal* upload, CreatedResourceMetal& created );
	ALResult CreateViews( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, const Tr2SubresourceData* initialData, const CreatedResourceMetal& created, Tr2PrimaryRenderContextAL& renderContext );

	std::unique_ptr<AsyncCreationMetal> m_asyncCreation;

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

	id<MTLBuffer> m_mtlReadBackBuffer;
	id<MTLTexture> m_mtlTexture;
	id<MTLTexture> m_mtlTextureSRGBView;
	uint32_t m_srvHeapIndices[2];
	std::vector<id<MTLTexture>> m_mtlTextureUAV;
	std::vector<uint32_t> m_uavHeapIndices;

	uint64_t m_usedInEncoder;

	MetalContext* m_metalContext;
	std::string m_name;
	Tr2MemoryCounterAL m_memory;
	bool m_wrappedTexture;
	friend class ::Tr2RenderContextAL;
};
}

#endif
