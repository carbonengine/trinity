#pragma once

#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"

namespace TrinityALImpl
{
	class Tr2TextureAL;
	class Tr2ResourceSetAL;
}


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;
struct Tr2BitmapDimensions;
struct Tr2SubresourceData;
struct Tr2MsaaDesc;
struct Tr2TextureSubresource;


class Tr2TextureAL
{
public:
	Tr2TextureAL();

	ALResult Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass();

	const Tr2BitmapDimensions& GetDesc() const;
	const Tr2MsaaDesc& GetMsaaDesc() const;
	Tr2GpuUsage::Type GetGpuUsage() const;
	Tr2CpuUsage::Type GetCpuUsage() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetMipCount() const;
	uint32_t GetTrueMipCount() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;
	Tr2RenderContextEnum::TextureType GetType() const;
	uint32_t GetArraySize() const;
	uint32_t GetMipSize( uint32_t mip ) const;

	bool operator==( const Tr2TextureAL& other ) const;

	ALResult MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
	void UnmapForReading( Tr2RenderContextAL& renderContext );
	ALResult MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
	void UnmapForWriting( Tr2RenderContextAL& renderContext );

	ALResult UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext );
	ALResult GenerateMipMaps( Tr2RenderContextAL& renderContext );
	ALResult Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext );
	uintptr_t GetSharedHandle() const;





	void Destroy();

	ALResult UpdateSubresource(
		uint32_t left,
		uint32_t top,
		uint32_t right,
		uint32_t bottom,
		const void* source,
		uint32_t sourcePitch,
		Tr2RenderContextAL& renderContext );

	ALResult Lock(
		uint32_t mipLevel,
		void*& data,
		uint32_t& pitch,
		Tr2RenderContextEnum::LockType,
		Tr2RenderContextAL& renderContext );
	ALResult Lock(
		uint32_t mipLevel,
		uint32_t* ltrb,
		void*& data,
		uint32_t& pitch,
		Tr2RenderContextEnum::LockType,
		Tr2RenderContextAL& renderContext );
	ALResult Lock(
		uint32_t face,
		uint32_t mipLevel,
		uint32_t* ltrb,
		void*& data,
		uint32_t& pitch,
		Tr2RenderContextEnum::LockType,
		Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion(
		uint32_t destX,
		uint32_t destY,
		Tr2TextureAL& source,
		uint32_t* ltrb,
		Tr2RenderContextAL& renderContext );
private:
	std::shared_ptr<TrinityALImpl::Tr2TextureAL> m_texture;

	friend class Tr2PrimaryRenderContextAL;
	friend class Tr2RenderContextAL;
	friend class Tr2SwapChainAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;
	friend class TrinityALImpl::Tr2TextureAL;
};

extern Tr2TextureAL nullRT;
extern Tr2TextureAL nullDS;
extern Tr2TextureAL nullTX;


#include TRINITY_AL_PLATFORM_INCLUDE( Tr2TextureAL )
