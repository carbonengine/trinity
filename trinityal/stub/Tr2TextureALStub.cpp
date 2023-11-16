#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2TextureALStub.h"
#include "Tr2RenderContextStub.h"


namespace TrinityALImpl
{
	Tr2TextureAL::Tr2TextureAL()
		:m_gpuUsage( Tr2GpuUsage::NONE ),
		m_cpuUsage( Tr2CpuUsage::NONE )
	{
	}

	ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( HasBufferFlags( gpuUsage ) )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		if( msaa.samples > 1 )
		{
			if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
			{
				return E_INVALIDARG;
			}
			if( cpuUsage != Tr2CpuUsage::NONE )
			{
				return E_INVALIDARG;
			}
			if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
		{
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				if( desc.GetArraySize() != 6 )
				{
					return E_INVALIDARG;
				}
			}
			else if( desc.GetArraySize() > 1 )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D && HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			return E_INVALIDARG;
		}
		if( msaa.samples > 1 && desc.GetTrueMipCount() > 1 )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) && desc.GetTrueMipCount() > 1 )
		{
			return E_INVALIDARG;
		}
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}
		if( !IsWritable( gpuUsage ) && !HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
		{
			return E_INVALIDARG;
		}

		m_desc = desc;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;
		m_msaa = msaa;

		return S_OK;
	}

	ALResult Tr2TextureAL::OpenShared( uintptr_t, Tr2GpuUsage::Type, Tr2PrimaryRenderContextAL& )
	{
		return E_FAIL;
	}

	void Tr2TextureAL::Destroy()
	{
		memset( &m_desc, 0, sizeof( m_desc ) );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_cpuUsage = Tr2CpuUsage::NONE;
	}

	bool Tr2TextureAL::IsValid() const
	{
		return m_desc.GetWidth() != 0;
	}

	Tr2ALMemoryType Tr2TextureAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	const Tr2BitmapDimensions& Tr2TextureAL::GetDesc() const
	{
		return m_desc;
	}

	const Tr2MsaaDesc& Tr2TextureAL::GetMsaaDesc() const
	{
		return m_msaa;
	}

	Tr2GpuUsage::Type Tr2TextureAL::GetGpuUsage() const
	{
		return m_gpuUsage;
	}

	Tr2CpuUsage::Type Tr2TextureAL::GetCpuUsage() const
	{
		return m_cpuUsage;
	}

	ALResult Tr2TextureAL::MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		data = nullptr;
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}

		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}

		auto mipPitch = m_desc.GetMipPitch( region.m_startMipLevel );
		auto size = mipPitch * m_desc.GetMipHeight( region.m_startMipLevel );

		if( m_data.size() != size )
		{
			m_data.resize( "Tr2TextureAL::m_data", size );
			if( m_data.empty() )
			{
				return E_FAIL;
			}
		}

		pitch = mipPitch;
		data = m_data.get();
		return S_OK;
	}

	void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& )
	{
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) && !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			m_data.clear();
		}
	}

	ALResult Tr2TextureAL::MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		data = nullptr;

		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}
		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}
		if( region.HasBox() && Tr2RenderContextEnum::IsCompressedFormat( m_desc.GetFormat() ) )
		{
			return E_INVALIDARG;
		}

		auto mipPitch = m_desc.GetMipPitch( region.m_startMipLevel );
		auto size = mipPitch * m_desc.GetMipHeight( region.m_startMipLevel );

		if( m_data.size() != size )
		{
			m_data.resize( "Tr2TextureAL::m_data", size );
			if( m_data.empty() )
			{
				return E_FAIL;
			}
		}

		pitch = mipPitch;
		data = m_data.get();
		return S_OK;
	}

	void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& )
	{
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) && !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			m_data.clear();
		}
	}

	ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void*, uint32_t, uint32_t, Tr2RenderContextAL& renderContext )
	{
		if( HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( m_gpuUsage ) )
		{
			return E_INVALIDCALL;
		}

		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}
		return S_OK;
	}

	ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !source.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( m_gpuUsage ) )
		{
			return E_INVALIDCALL;
		}

		if( destSubresource.IsSubresourceFull( m_desc ) && sourceSubresource.IsSubresourceFull( source.m_desc ) )
		{
			return S_OK;
		}

		Tr2TextureSubresource src = sourceSubresource;
		Tr2TextureSubresource dst = destSubresource;

		if( !Crop( src, source.m_desc, dst, m_desc ) )
		{
			return E_FAIL;
		}

		return S_OK;
	}

	ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& )
	{
		if( !HasFlag( m_gpuUsage, Tr2GpuUsage::RENDER_TARGET ) || !HasFlag( m_gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			return E_INVALIDCALL;
		}
		return S_OK;
	}

	ALResult Tr2TextureAL::Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext )
	{
		if( m_msaa.samples <= 1 )
		{
			return destination.CopySubresourceRegion( Tr2TextureSubresource(), *this, Tr2TextureSubresource(), renderContext );
		}

		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !destination.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( destination.m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( destination.m_gpuUsage ) )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetWidth() != destination.m_desc.GetWidth() || m_desc.GetHeight() != destination.m_desc.GetHeight() )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetFormat() != destination.m_desc.GetFormat() )
		{
			return E_INVALIDARG;
		}
		if( destination.m_msaa.samples > 1 )
		{
			return E_INVALIDARG;
		}

		return S_OK;
	}

	uintptr_t Tr2TextureAL::GetSharedHandle() const
	{
		return 0;
	}

	void Tr2TextureAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
	{
	}

	ALResult Tr2TextureAL::SetName( const char* )
	{
		return S_OK;
	}
}

#endif