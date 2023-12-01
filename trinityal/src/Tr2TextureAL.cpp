#include "StdAfx.h"
#include "../include/Tr2TextureAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2TextureAL )

bool g_preloadTextureToDeviceOnPrepare = true;


namespace
{
	std::shared_ptr<TrinityALImpl::Tr2TextureAL> nullTexture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
}


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

ALResult Tr2TextureAL::OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
{
	m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	auto hr = m_texture->OpenShared( handle, gpuUsage, renderContext );
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

namespace
{
	bool FormatIsBGR( Tr2RenderContextEnum::PixelFormat format )
	{
		switch( format )
		{
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM:
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM:
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}
}

ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( GetTrueMipCount() <= 1 )
	{
		return S_OK;
	}
	if( FormatIsBGR( GetFormat() ) )
	{
		// ATM we don't support BGR formats in DirectX12 and don't seem to be using them with GenerateMipMaps
		// so we just fail here. If needed the support can be added later.
		return E_INVALIDCALL;
	}
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

ALResult Tr2TextureAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_texture->SetName( name );
}

TrinityALImpl::Tr2TextureAL* Tr2TextureAL::TrinityALImpl_GetObject() const
{
	return m_texture.get();
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
