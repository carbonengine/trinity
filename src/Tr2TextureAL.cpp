#include "StdAfx.h"
#include "../include/Tr2TextureAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2TextureAL )

bool g_preloadTextureToDeviceOnPrepare = true;


namespace
{
	std::shared_ptr<TrinityALImpl::Tr2TextureAL> nullTexture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
}

Tr2TextureAL nullRT;
Tr2TextureAL nullDS;
Tr2TextureAL nullTX;


Tr2TextureAL::Tr2TextureAL()
	:m_texture( nullTexture )
{
}

ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( desc, Tr2MsaaDesc(), gpuUsage, Tr2CpuUsage::NONE, nullptr, renderContext );
}

ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( desc, msaa, gpuUsage, Tr2CpuUsage::NONE, nullptr, renderContext );
}

ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( desc, Tr2MsaaDesc(), gpuUsage, Tr2CpuUsage::NONE, initialData, renderContext );
}

ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( desc, Tr2MsaaDesc(), gpuUsage, cpuUsage, nullptr, renderContext );
}

ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( desc, Tr2MsaaDesc(), gpuUsage, cpuUsage, initialData, renderContext );
}

ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
{
	m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	auto hr = m_texture->Create( desc, msaa, gpuUsage, cpuUsage, initialData, renderContext );
	if( FAILED( hr ) )
	{
		m_texture = nullTexture;
	}
	return hr;
}

bool Tr2TextureAL::IsValid() const
{
	return m_texture->IsValid();
}

Tr2ALMemoryType Tr2TextureAL::GetMemoryClass()
{
	return m_texture->GetMemoryClass();
}

const Tr2BitmapDimensions& Tr2TextureAL::GetDesc() const
{
	return m_texture->GetDesc();
}

const Tr2MsaaDesc& Tr2TextureAL::GetMsaaDesc() const
{
	return m_texture->GetMsaaDesc();
}

Tr2GpuUsage::Type Tr2TextureAL::GetGpuUsage() const
{
	return m_texture->GetGpuUsage();
}

Tr2CpuUsage::Type Tr2TextureAL::GetCpuUsage() const
{
	return m_texture->GetCpuUsage();
}

uint32_t Tr2TextureAL::GetWidth() const
{
	return m_texture->GetDesc().GetWidth();
}

uint32_t Tr2TextureAL::GetHeight() const
{
	return m_texture->GetDesc().GetHeight();
}

uint32_t Tr2TextureAL::GetMipCount() const
{
	return m_texture->GetDesc().GetMipCount();
}

uint32_t Tr2TextureAL::GetTrueMipCount() const
{
	return m_texture->GetDesc().GetTrueMipCount();
}

Tr2RenderContextEnum::PixelFormat Tr2TextureAL::GetFormat() const
{
	return m_texture->GetDesc().GetFormat();
}

Tr2RenderContextEnum::TextureType Tr2TextureAL::GetType() const
{
	return m_texture->GetDesc().GetType();
}

uint32_t Tr2TextureAL::GetArraySize() const
{
	return m_texture->GetDesc().GetArraySize();
}

uint32_t Tr2TextureAL::GetMipSize( uint32_t mip ) const
{
	return m_texture->GetDesc().GetMipSize( mip );
}

bool Tr2TextureAL::operator==( const Tr2TextureAL& other ) const
{
	return m_texture == other.m_texture;
}

ALResult Tr2TextureAL::MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
{
	return m_texture->MapForReading( region, data, pitch, renderContext );
}

void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& renderContext )
{
	m_texture->UnmapForReading( renderContext );
}

ALResult Tr2TextureAL::MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
{
	return m_texture->MapForWriting( region, data, pitch, renderContext );
}

void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
{
	m_texture->UnmapForWriting( renderContext );
}

ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext )
{
	return m_texture->UpdateSubresource( region, source, pitch, slicePitch, renderContext );
}

ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext )
{
	return m_texture->CopySubresourceRegion( destSubresource, *source.m_texture, sourceSubresource, renderContext );
}

ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
{
	return m_texture->GenerateMipMaps( renderContext );
}

ALResult Tr2TextureAL::Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext )
{
	return m_texture->Resolve( *destination.m_texture, renderContext );
}

uintptr_t Tr2TextureAL::GetSharedHandle() const
{
	return m_texture->GetSharedHandle();
}


namespace
{
	void ConvertUsage( Tr2RenderContextEnum::BufferUsage usage, Tr2GpuUsage::Type& gpuUsage, Tr2CpuUsage::Type& cpuUsage )
	{
		gpuUsage = Tr2GpuUsage::SHADER_RESOURCE;
		cpuUsage = Tr2CpuUsage::NONE;

		if( ( usage & Tr2RenderContextEnum::USAGE_IMMUTABLE ) != 0 )
		{
			cpuUsage = cpuUsage | Tr2CpuUsage::READ;
		}
		else if( ( usage & Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY ) != 0 )
		{
			cpuUsage = cpuUsage | Tr2CpuUsage::WRITE_OFTEN;
		}
		else
		{
			cpuUsage = cpuUsage | Tr2CpuUsage::WRITE;
		}
		if( ( usage & Tr2RenderContextEnum::USAGE_CPU_READ ) != 0 )
		{
			cpuUsage = cpuUsage | Tr2CpuUsage::READ;
		}
		if( ( usage & Tr2RenderContextEnum::USAGE_UNORDERED_ACCESS ) != 0 )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::UNORDERED_ACCESS;
		}
		if( ( usage & Tr2RenderContextEnum::USAGE_SHADER_RESOURCE ) != 0 )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::SHARED;
		}
	}
}


void Tr2TextureAL::Destroy()
{
	m_texture = nullTexture;
}

ALResult Tr2TextureAL::UpdateSubresource(
	uint32_t left,
	uint32_t top,
	uint32_t right,
	uint32_t bottom,
	const void* source,
	uint32_t sourcePitch,
	Tr2RenderContextAL& renderContext )
{
	return UpdateSubresource( Tr2TextureSubresource( 0 ).SetRect( left, top, right, bottom ), source, sourcePitch, 0, renderContext );
}

ALResult Tr2TextureAL::Lock(
	uint32_t mipLevel,
	void*& data,
	uint32_t& pitch,
	Tr2RenderContextEnum::LockType lockType,
	Tr2RenderContextAL& renderContext )
{
	if( lockType == Tr2RenderContextEnum::LOCK_NO_OVERWRITE )
	{
		return E_INVALIDARG;
	}
	if( lockType == Tr2RenderContextEnum::LOCK_READONLY )
	{
		return MapForReading( Tr2TextureSubresource( mipLevel ), *(const void**)&data, pitch, renderContext );
	}
	else
	{
		return MapForWriting( Tr2TextureSubresource( mipLevel ), data, pitch, renderContext );
	}
}

ALResult Tr2TextureAL::Lock(
	uint32_t mipLevel,
	uint32_t* ltrb,
	void*& data,
	uint32_t& pitch,
	Tr2RenderContextEnum::LockType lockType,
	Tr2RenderContextAL& renderContext )
{
	if( lockType == Tr2RenderContextEnum::LOCK_NO_OVERWRITE )
	{
		return E_INVALIDARG;
	}
	if( lockType == Tr2RenderContextEnum::LOCK_READONLY )
	{
		return MapForReading( Tr2TextureSubresource( mipLevel ).SetRect( ltrb ), *(const void**)&data, pitch, renderContext );
	}
	else
	{
		return MapForWriting( Tr2TextureSubresource( mipLevel ).SetRect( ltrb ), data, pitch, renderContext );
	}
}

ALResult Tr2TextureAL::Lock(
	uint32_t face,
	uint32_t mipLevel,
	uint32_t* ltrb,
	void*& data,
	uint32_t& pitch,
	Tr2RenderContextEnum::LockType lockType,
	Tr2RenderContextAL& renderContext )
{
	if( lockType == Tr2RenderContextEnum::LOCK_NO_OVERWRITE )
	{
		return E_INVALIDARG;
	}
	if( lockType == Tr2RenderContextEnum::LOCK_READONLY )
	{
		return MapForReading( Tr2TextureSubresource( face, mipLevel ).SetRect( ltrb ), *(const void**)&data, pitch, renderContext );
	}
	else
	{
		return MapForWriting( Tr2TextureSubresource( face, mipLevel ).SetRect( ltrb ), data, pitch, renderContext );
	}
}

ALResult Tr2TextureAL::CopySubresourceRegion(
	uint32_t destX,
	uint32_t destY,
	Tr2TextureAL& source,
	uint32_t* ltrb,
	Tr2RenderContextAL& renderContext )
{
	Tr2TextureSubresource src;
	if( ltrb )
	{
		src.SetRect( ltrb );
	}
	Tr2TextureSubresource dst;
	if( ltrb )
	{
		dst.SetRect( destX, destY, destX + ( src.m_right - src.m_left ), destY + ( src.m_bottom - src.m_top ) );
	}
	else
	{
		dst.SetRect( destX, destY, destX + source.GetWidth(), destY + source.GetHeight() );
	}
	return CopySubresourceRegion( dst, source, src, renderContext );
}