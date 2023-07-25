#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2TextureALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"
#include "../ALLog.h"

namespace
{
	std::vector<D3D11_SUBRESOURCE_DATA> ConvertInitialData( uint32_t numInit, Tr2SubresourceData* initialData )
	{
		std::vector<D3D11_SUBRESOURCE_DATA> init( numInit );
		if( initialData )
		{
			for( size_t i = 0; i != init.size(); ++i )
			{
				init[i].pSysMem = initialData[i].m_sysMem;
				init[i].SysMemPitch = initialData[i].m_sysMemPitch;
				init[i].SysMemSlicePitch = initialData[i].m_sysMemSlicePitch;
			}
		}

		return init;
	}

	DXGI_FORMAT GetSrvFormat( Tr2RenderContextEnum::PixelFormat format )
	{
		switch( format )
		{
		case Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		default:
			return DXGI_FORMAT( format );
		}
	}

	ALResult Create1D( CComPtr<ID3D11Resource>& texture, const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		D3D11_TEXTURE1D_DESC desc1D;
		memset( &desc1D, 0, sizeof( desc1D ) );
		desc1D.Width = desc.GetWidth();
		desc1D.MipLevels = desc.GetTrueMipCount(); // see below, no auto-gen yet
		desc1D.ArraySize = desc.GetArraySize();
		desc1D.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeTypeless( desc.GetFormat() ) );

		if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			desc1D.Usage = D3D11_USAGE_DYNAMIC;
			desc1D.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			desc1D.Usage = D3D11_USAGE_DEFAULT;
			desc1D.CPUAccessFlags = 0;
		}
		else if( IsWritable( gpuUsage ) )
		{
			desc1D.Usage = D3D11_USAGE_DEFAULT;
		}
		else
		{
			desc1D.Usage = D3D11_USAGE_IMMUTABLE;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			desc1D.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			desc1D.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if( desc.GetTrueMipCount() > 1 && HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
			{
				desc1D.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			}
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			desc1D.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			desc1D.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) )
		{
			desc1D.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		}

		auto init = ConvertInitialData( desc1D.MipLevels * desc1D.ArraySize, initialData );

		CComPtr<ID3D11Texture1D> texture1D;

		FORWARD_HR( renderContext.m_d3dDevice11->CreateTexture1D( &desc1D, initialData ? &init[0] : nullptr, &texture1D ) );
		texture = texture1D;
		return S_OK;
	}

	ALResult Create2D( CComPtr<ID3D11Resource>& texture, const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		D3D11_TEXTURE2D_DESC desc2D;
		memset( &desc2D, 0, sizeof( desc2D ) );
		desc2D.Width = desc.GetWidth();
		desc2D.Height = desc.GetHeight();
		desc2D.MipLevels = desc.GetTrueMipCount();	// see below, no auto-gen yet
		desc2D.ArraySize = desc.GetArraySize();
		desc2D.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeTypeless( desc.GetFormat() ) );

		if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			desc2D.Usage = D3D11_USAGE_DYNAMIC;
			desc2D.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			desc2D.Usage = D3D11_USAGE_DEFAULT;
			desc2D.CPUAccessFlags = 0;
		}
		else if( IsWritable( gpuUsage ) )
		{
			desc2D.Usage = D3D11_USAGE_DEFAULT;
		}
		else
		{
			desc2D.Usage = D3D11_USAGE_IMMUTABLE;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			desc2D.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			desc2D.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if( desc.GetTrueMipCount() > 1 && HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
			{
				desc2D.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			}
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			desc2D.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			desc2D.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) )
		{
			desc2D.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		}

		desc2D.SampleDesc.Count = msaa.samples;
		desc2D.SampleDesc.Quality = msaa.quality;
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
		{
			desc2D.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		}

		auto init = ConvertInitialData( desc2D.MipLevels * desc2D.ArraySize, initialData );

		CComPtr<ID3D11Texture2D> texture2D;

		FORWARD_HR( renderContext.m_d3dDevice11->CreateTexture2D( &desc2D, initialData ? &init[0] : nullptr, &texture2D ) );
		texture = texture2D;
		return S_OK;
	}

	ALResult Create3D( CComPtr<ID3D11Resource>& texture, const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		CCP_UNUSED( msaa );

		D3D11_TEXTURE3D_DESC desc3D;
		memset( &desc3D, 0, sizeof( desc3D ) );
		desc3D.Width = desc.GetWidth();
		desc3D.Height = desc.GetHeight();
		desc3D.Depth = desc.GetDepth();
		desc3D.MipLevels = desc.GetTrueMipCount();
		desc3D.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeTypeless( desc.GetFormat() ) );

		if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			desc3D.Usage = D3D11_USAGE_DYNAMIC;
			desc3D.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			desc3D.Usage = D3D11_USAGE_DEFAULT; // or D3D11_USAGE_DYNAMIC?
			desc3D.CPUAccessFlags = 0;
		}
		else if( IsWritable( gpuUsage ) )
		{
			desc3D.Usage = D3D11_USAGE_DEFAULT;
		}
		else
		{
			desc3D.Usage = D3D11_USAGE_IMMUTABLE;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			desc3D.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			desc3D.BindFlags |= D3D11_BIND_RENDER_TARGET;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			desc3D.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			desc3D.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) )
		{
			desc3D.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		}

		if( !desc.GetMipCount() )
		{
			desc3D.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		auto init = ConvertInitialData( desc3D.MipLevels, initialData );

		CComPtr<ID3D11Texture3D> texture3D;

		FORWARD_HR( renderContext.m_d3dDevice11->CreateTexture3D( &desc3D, initialData ? &init[0] : nullptr, &texture3D ) );
		texture = texture3D;
		return S_OK;
	}


	bool IsTypeless( Tr2RenderContextEnum::PixelFormat format )
	{
		using namespace Tr2RenderContextEnum;
		return format == PIXEL_FORMAT_R32G32B32A32_TYPELESS ||
			format == PIXEL_FORMAT_R32G32B32_TYPELESS ||
			format == PIXEL_FORMAT_R16G16B16A16_TYPELESS ||
			format == PIXEL_FORMAT_R32G32_TYPELESS ||
			format == PIXEL_FORMAT_R10G10B10A2_TYPELESS ||
			format == PIXEL_FORMAT_R8G8B8A8_TYPELESS ||
			format == PIXEL_FORMAT_R16G16_TYPELESS ||
			format == PIXEL_FORMAT_R32_TYPELESS ||
			format == PIXEL_FORMAT_R8G8_TYPELESS ||
			format == PIXEL_FORMAT_R16_TYPELESS ||
			format == PIXEL_FORMAT_R8_TYPELESS ||
			format == PIXEL_FORMAT_BC1_TYPELESS ||
			format == PIXEL_FORMAT_BC2_TYPELESS ||
			format == PIXEL_FORMAT_BC3_TYPELESS ||
			format == PIXEL_FORMAT_BC4_TYPELESS ||
			format == PIXEL_FORMAT_BC5_TYPELESS ||
			format == PIXEL_FORMAT_BC6H_TYPELESS ||
			format == PIXEL_FORMAT_BC7_TYPELESS ||
			format == PIXEL_FORMAT_B8G8R8A8_TYPELESS ||
			format == PIXEL_FORMAT_B8G8R8X8_TYPELESS;
	}

	Tr2RenderContextEnum::PixelFormat FromTypeless( Tr2RenderContextEnum::PixelFormat format )
	{
		using namespace Tr2RenderContextEnum;
		switch( format )
		{
		case PIXEL_FORMAT_R32G32B32A32_TYPELESS:
			return PIXEL_FORMAT_R32G32B32A32_FLOAT;
		case PIXEL_FORMAT_R32G32B32_TYPELESS:
			return PIXEL_FORMAT_R32G32B32_FLOAT;

		case PIXEL_FORMAT_R16G16B16A16_TYPELESS:
			return PIXEL_FORMAT_R16G16B16A16_FLOAT;

		case PIXEL_FORMAT_R32G32_TYPELESS:
			return PIXEL_FORMAT_R32G32_FLOAT;

		case PIXEL_FORMAT_R10G10B10A2_TYPELESS:
			return PIXEL_FORMAT_R10G10B10A2_UNORM;

		case PIXEL_FORMAT_R8G8B8A8_TYPELESS:
			return PIXEL_FORMAT_R8G8B8A8_UNORM;

		case PIXEL_FORMAT_R16G16_TYPELESS:
			return PIXEL_FORMAT_R16G16_FLOAT;

		case PIXEL_FORMAT_R32_TYPELESS:
			return PIXEL_FORMAT_R32_FLOAT;

		case PIXEL_FORMAT_R8G8_TYPELESS:
			return PIXEL_FORMAT_R8G8_UNORM;

		case PIXEL_FORMAT_R16_TYPELESS:
			return PIXEL_FORMAT_R16_FLOAT;

		case PIXEL_FORMAT_R8_TYPELESS:
			return PIXEL_FORMAT_R8_UNORM;

		case PIXEL_FORMAT_BC1_TYPELESS:
			return PIXEL_FORMAT_BC1_UNORM;
		case PIXEL_FORMAT_BC2_TYPELESS:
			return PIXEL_FORMAT_BC2_UNORM;
		case PIXEL_FORMAT_BC3_TYPELESS:
			return PIXEL_FORMAT_BC3_UNORM;
		case PIXEL_FORMAT_BC4_TYPELESS:
			return PIXEL_FORMAT_BC4_UNORM;
		case PIXEL_FORMAT_BC5_TYPELESS:
			return PIXEL_FORMAT_BC5_UNORM;
		case PIXEL_FORMAT_BC6H_TYPELESS:
			return PIXEL_FORMAT_BC6H_SF16;
		case PIXEL_FORMAT_BC7_TYPELESS:
			return PIXEL_FORMAT_BC7_UNORM;

		case PIXEL_FORMAT_B8G8R8A8_TYPELESS:
			return PIXEL_FORMAT_B8G8R8A8_UNORM;
		case PIXEL_FORMAT_B8G8R8X8_TYPELESS:
			return PIXEL_FORMAT_B8G8R8X8_UNORM;

		default:
			return format;
		}
	}
}

namespace TrinityALImpl
{
	Tr2TextureAL::Tr2TextureAL()
		:m_gpuUsage( Tr2GpuUsage::NONE ),
		m_cpuUsage( Tr2CpuUsage::NONE ),
		m_lockedSubresource( 0 ),
		m_writeBox{ 0, 0, 0, 0, 0, 0 }
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

		CComPtr<ID3D11Resource> texture;
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_1D )
		{
			FORWARD_HR( Create1D( texture, desc, msaa, gpuUsage, cpuUsage, initialData, renderContext ) );
		}
		else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
		{
			FORWARD_HR( Create3D( texture, desc, msaa, gpuUsage, cpuUsage, initialData, renderContext ) );
		}
		else
		{
			FORWARD_HR( Create2D( texture, desc, msaa, gpuUsage, cpuUsage, initialData, renderContext ) );
		}

		// Behold! A workaround for WARP device crash when creating multiple SRVs into a BC7 texture!
		if( !renderContext.m_dxgiOutput && desc.GetFormat() == Tr2RenderContextEnum::PIXEL_FORMAT_BC7_UNORM )
		{
			// Create a single linear view into the texture
			FORWARD_HR( CreateViews( texture, desc, msaa, gpuUsage, cpuUsage, false, renderContext ) );

			// For sRGB view we create a copy of BC7 texture with an sRGB view into it
			if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
			{
				CComPtr<ID3D11Resource> srgbTexture;
				FORWARD_HR( Create2D( srgbTexture, desc, msaa, gpuUsage, cpuUsage, initialData, renderContext ) );

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				memset( &srvDesc, 0, sizeof( srvDesc ) );

				srvDesc.Format = GetSrvFormat( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );

				if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
				{
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
					srvDesc.Texture3D.MipLevels = desc.GetTrueMipCount();
				}
				else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
				{
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.TextureCube.MipLevels = desc.GetTrueMipCount();
				}
				else if( msaa.samples > 1 )
				{
					srvDesc.ViewDimension = desc.GetArraySize() > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DMS;
					srvDesc.Texture2DMSArray.ArraySize = desc.GetArraySize();
				}
				else
				{
					srvDesc.ViewDimension = desc.GetArraySize() > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = desc.GetTrueMipCount();
					srvDesc.Texture2DArray.ArraySize = desc.GetArraySize();
				}
				m_view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = nullptr;
				FORWARD_HR( renderContext.m_d3dDevice11->CreateShaderResourceView( srgbTexture, &srvDesc, &m_view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] ) );
			}
		}
		else
		{
			FORWARD_HR( CreateViews( texture, desc, msaa, gpuUsage, cpuUsage, true, renderContext ) );
		}

		m_desc = desc;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;
		m_msaa = msaa;

		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, m_desc, msaa );

		return S_OK;
	}

	ALResult Tr2TextureAL::OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		CCP_AL_LOG( "Opening shared texture %lld", uint64_t( handle ) );

		CComPtr<ID3D11Texture2D> resource;
		auto hr = renderContext.m_d3dDevice11->OpenSharedResource( reinterpret_cast<HANDLE>( handle ), __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &resource.p ) );
		if( FAILED( hr ) )
		{
			CCP_AL_LOG( "Failed to open shared texture %lld", uint64_t( handle ) );
		}

		FORWARD_HR( hr );

		if( !resource )
		{
			return E_FAIL;
		}

		D3D11_TEXTURE2D_DESC dxDesc;
		resource->GetDesc( &dxDesc );
		Tr2RenderContextEnum::PixelFormat fmt;
		bool createSrgb = true;
		if( IsTypeless( Tr2RenderContextEnum::PixelFormat( dxDesc.Format ) ) )
		{
			fmt = FromTypeless( Tr2RenderContextEnum::PixelFormat( dxDesc.Format ) );
		}
		else
		{
			fmt = Tr2RenderContextEnum::PixelFormat( dxDesc.Format );
			createSrgb = false;
		}
		Tr2BitmapDimensions desc( Tr2RenderContextEnum::TEX_TYPE_2D, fmt, dxDesc.Width, dxDesc.Height, 1, dxDesc.MipLevels );

		CCP_AL_LOG( "Creating views for the shared texture %lld, GPU usage is %d", uint64_t( handle ), int( gpuUsage ) );

		hr = CreateViews( resource, desc, Tr2MsaaDesc(), gpuUsage, Tr2CpuUsage::NONE, createSrgb, renderContext );
		if( FAILED( hr ) )
		{
			CCP_AL_LOG( "Failed to create views for the shared texture %lld, GPU usage is %d", uint64_t( handle ), int( gpuUsage ) );
		}
		FORWARD_HR( hr );

		m_desc = desc;
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = gpuUsage;
		m_cpuUsage = Tr2CpuUsage::NONE;
		m_texture = resource;
		return S_OK;
	}

	ALResult Tr2TextureAL::CreateViews( 
		ID3D11Resource* texture, 
		const Tr2BitmapDimensions& desc, 
		const Tr2MsaaDesc& msaa, 
		Tr2GpuUsage::Type gpuUsage, 
		Tr2CpuUsage::Type cpuUsage, 
		bool createSrgb,
		Tr2PrimaryRenderContextAL& renderContext )
	{
		CCP_UNUSED( cpuUsage );

		CComPtr<ID3D11ShaderResourceView> view[Tr2RenderContextEnum::_COLOR_SPACE_COUNT];
		std::vector<CComPtr<ID3D11RenderTargetView>> renderTargets;
		CComPtr<ID3D11DepthStencilView> depthStencil[DepthOption::COUNT];
		std::vector<CComPtr<ID3D11UnorderedAccessView>> uav;

		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			memset( &srvDesc, 0, sizeof( srvDesc ) );

			srvDesc.Format = GetSrvFormat( desc.GetFormat() );

			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_1D )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MipLevels = desc.GetTrueMipCount();
			}
			else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MipLevels = desc.GetTrueMipCount();
			}
			else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MipLevels = desc.GetTrueMipCount();
			}
			else if( msaa.samples > 1 )
			{
				srvDesc.ViewDimension = desc.GetArraySize() > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DMS;
				srvDesc.Texture2DMSArray.ArraySize = desc.GetArraySize();
			}
			else
			{
				srvDesc.ViewDimension = desc.GetArraySize() > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = desc.GetTrueMipCount();
				srvDesc.Texture2DArray.ArraySize = desc.GetArraySize();
			}
			FORWARD_HR( renderContext.m_d3dDevice11->CreateShaderResourceView( texture, &srvDesc, &view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR] ) );
			if( createSrgb )
			{
				srvDesc.Format = GetSrvFormat( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );
				if( FAILED( renderContext.m_d3dDevice11->CreateShaderResourceView( texture, &srvDesc, &view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] ) ) )
				{
					CCP_AL_LOGWARN( "Failed to create an sRGB view for the texture of format %i - will use the linear view instead", int( srvDesc.Format ) );
					view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR];
				}
			}
			else
			{
				view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR];
			}
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
			memset( &rtvDesc, 0, sizeof( rtvDesc ) );

			rtvDesc.Format = static_cast<DXGI_FORMAT>( desc.GetFormat() );
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_1D )
			{
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
			}	
			else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
			{
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
				rtvDesc.Texture3D.WSize = 1;
			}
			else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				rtvDesc.Texture2DArray.ArraySize = 1;
			}
			else if( msaa.samples > 1 )
			{
				rtvDesc.ViewDimension = desc.GetArraySize() > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D11_RTV_DIMENSION_TEXTURE2DMS;
				rtvDesc.Texture2DMSArray.ArraySize = 1;
			}
			else
			{
				rtvDesc.ViewDimension = desc.GetArraySize() > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2DArray.ArraySize = 1;
			}

			renderTargets.resize( 2 );
			FORWARD_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvDesc, &renderTargets[Tr2RenderContextEnum::COLOR_SPACE_LINEAR] ) );
			if( createSrgb )
			{
				rtvDesc.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );
				FORWARD_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvDesc, &renderTargets[Tr2RenderContextEnum::COLOR_SPACE_SRGB] ) );
			}
			else
			{
				renderTargets[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = renderTargets[Tr2RenderContextEnum::COLOR_SPACE_LINEAR];
			}

			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
			{
				renderTargets.resize( desc.GetDepth() * 2 );
				for( uint32_t i = 1; i < desc.GetDepth(); ++i )
				{
					rtvDesc.Texture3D.FirstWSlice = i;
					FORWARD_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvDesc, &renderTargets[Tr2RenderContextEnum::COLOR_SPACE_LINEAR + i * 2] ) );
					if( createSrgb )
					{
						rtvDesc.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );
						FORWARD_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvDesc, &renderTargets[Tr2RenderContextEnum::COLOR_SPACE_SRGB + i * 2] ) );
					}
					else
					{
						renderTargets[Tr2RenderContextEnum::COLOR_SPACE_SRGB + i * 2] = renderTargets[Tr2RenderContextEnum::COLOR_SPACE_LINEAR + i * 2];
					}
				}
			}
			else if( desc.GetArraySize() > 1 )
			{
				renderTargets.resize( desc.GetArraySize() * 2 );
				for( uint32_t i = 1; i < desc.GetArraySize(); ++i )
				{
					rtvDesc.Texture2DArray.FirstArraySlice = i;
					FORWARD_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvDesc, &renderTargets[Tr2RenderContextEnum::COLOR_SPACE_LINEAR + i * 2] ) );
					if( createSrgb )
					{
						rtvDesc.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );
						FORWARD_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvDesc, &renderTargets[Tr2RenderContextEnum::COLOR_SPACE_SRGB + i * 2] ) );
					}
					else
					{
						renderTargets[Tr2RenderContextEnum::COLOR_SPACE_SRGB + i * 2] = renderTargets[Tr2RenderContextEnum::COLOR_SPACE_LINEAR + i * 2];
					}
				}
			}

		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
			memset( &descDSV, 0, sizeof( descDSV ) );
			descDSV.Format = static_cast<DXGI_FORMAT>( desc.GetFormat() );
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				descDSV.Texture2DArray.ArraySize = 6;
			}
			else if( msaa.samples > 1 )
			{
				descDSV.ViewDimension = desc.GetArraySize() > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D11_DSV_DIMENSION_TEXTURE2DMS;
				descDSV.Texture2DMSArray.ArraySize = desc.GetArraySize();
			}
			else
			{
				descDSV.ViewDimension = desc.GetArraySize() > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2D;
				descDSV.Texture2DArray.ArraySize = desc.GetArraySize();
			}

			FORWARD_HR( renderContext.m_d3dDevice11->CreateDepthStencilView( texture, &descDSV, &depthStencil[DepthOption::READ_WRITE] ) );
			descDSV.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
			FORWARD_HR( renderContext.m_d3dDevice11->CreateDepthStencilView( texture, &descDSV, &depthStencil[DepthOption::READ_ONLY] ) );
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;
			memset( &descUAV, 0, sizeof( descUAV ) );
			descUAV.Format = static_cast<DXGI_FORMAT>( desc.GetFormat() );
			uav.resize( desc.GetTrueMipCount() );
			for( uint32_t mip = 0; mip < desc.GetTrueMipCount(); ++mip )
			{
				if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_1D )
				{
					descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
					descUAV.Texture1D.MipSlice = mip;
				}
				else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
				{
					descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
					descUAV.Texture3D.WSize = (UINT)-1;
					descUAV.Texture3D.MipSlice = mip;
				}
				else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
				{
					descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
					descUAV.Texture2DArray.ArraySize = 6;
					descUAV.Texture2DArray.MipSlice = mip;
				}
				else if( desc.GetArraySize() > 1 )
				{
					descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
					descUAV.Texture2DArray.ArraySize = desc.GetArraySize();
					descUAV.Texture2DArray.MipSlice = mip;
				}
				else
				{
					descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
					descUAV.Texture2D.MipSlice = mip;
				}
				FORWARD_HR( renderContext.m_d3dDevice11->CreateUnorderedAccessView( texture, &descUAV, &uav[mip] ) );
			}
		}

		m_texture = texture;
		m_view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR] = view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR];
		m_view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = view[Tr2RenderContextEnum::COLOR_SPACE_SRGB];
		m_renderTarget = renderTargets;
		m_depthStencil[DepthOption::READ_WRITE] = depthStencil[DepthOption::READ_WRITE];
		m_depthStencil[DepthOption::READ_ONLY] = depthStencil[DepthOption::READ_ONLY];
		m_uav = uav;
		return S_OK;
	}

	void Tr2TextureAL::Destroy()
	{
		m_texture = nullptr;
		m_view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR] = nullptr;
		m_view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = nullptr;
		m_renderTarget.clear();
		m_depthStencil[DepthOption::READ_WRITE] = nullptr;
		m_depthStencil[DepthOption::READ_ONLY] = nullptr;
		m_uav.clear();
		m_stagingTexture = nullptr;
		m_memory.Reset();
		memset( &m_desc, 0, sizeof( m_desc ) );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_cpuUsage = Tr2CpuUsage::NONE;
		m_writeStaging.clear();
		m_lockedSubresource = 0;
		m_writeBox = { 0, 0, 0, 0, 0, 0 };
	}

	bool Tr2TextureAL::IsValid() const
	{
		return m_texture != nullptr;
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
		if( m_desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
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

		if( !m_stagingTexture )
		{
			D3D11_TEXTURE2D_DESC desc;
			memset( &desc, 0, sizeof( desc ) );
			desc.Width = m_desc.GetWidth();
			desc.Height = m_desc.GetHeight();
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = static_cast<DXGI_FORMAT>( Tr2RenderContextEnum::MakeTypeless( m_desc.GetFormat() ) );

			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

			desc.SampleDesc.Count = 1;

			if( !renderContext.m_secondaryDevice11 )
			{
				return E_FAIL;
			}
			FORWARD_HR( renderContext.m_secondaryDevice11->CreateTexture2D( &desc, nullptr, &m_stagingTexture ) );
			if( !m_stagingTexture )
			{
				return E_FAIL;
			}
		}

		//renderContext.m_context->CopyResource( m_staging, m_texture );
		if( region.HasBox() )
		{
			D3D11_BOX box = { region.m_box.left, region.m_box.top, region.m_box.front, region.m_box.right, region.m_box.bottom, region.m_box.back };
			renderContext.m_context->CopySubresourceRegion( m_stagingTexture, 0, 0, 0, 0, m_texture, D3D10CalcSubresource( region.m_startMipLevel, region.m_startFace, m_desc.GetTrueMipCount() ), &box );
		}
		else
		{
			renderContext.m_context->CopySubresourceRegion( m_stagingTexture, 0, 0, 0, 0, m_texture, D3D10CalcSubresource( region.m_startMipLevel, region.m_startFace, m_desc.GetTrueMipCount() ), nullptr );
		}

		D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
		HRESULT hr = renderContext.m_context->Map( m_stagingTexture, 0, D3D11_MAP_READ, 0, &ms );
		data = ms.pData;
		pitch = ms.RowPitch;
		if( !ms.pData )
		{
			return E_FAIL;
		}
		return hr;
	}

	void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
		if( !m_stagingTexture || !renderContext.IsValid() )
		{
			return;
		}
		renderContext.m_context->Unmap( m_stagingTexture, 0 );
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) )
		{
			m_stagingTexture = nullptr;
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

		if( HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
			HRESULT hr = renderContext.m_context->Map( m_texture, D3D10CalcSubresource( region.m_startMipLevel, region.m_startFace, m_desc.GetTrueMipCount() ), D3D11_MAP_WRITE_DISCARD, 0, &ms );
			if( FAILED( hr ) )
			{
				pitch = 0;
				return hr;
			}
			if( !ms.pData )
			{
				pitch = 0;
				return E_FAIL;
			}
			if( region.HasBox() )
			{
				ms.pData = static_cast<uint8_t*>( ms.pData ) + region.m_box.left * Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() ) + region.m_box.top * ms.RowPitch + region.m_box.front * ms.DepthPitch;
			}
#if TRINITY_AL_GUARD_LOCKS
			size_t size;
			if( region.HasBox() )
			{
				size = ms.RowPitch * ( region.m_box.bottom - region.m_box.top );
			}
			else
			{
				size = ms.RowPitch * m_desc.GetMipHeight( region.m_startMipLevel );
			}

			m_lockGuard.Lock( size, ms.pData );
			data = m_lockGuard.GetMemory();
			pitch = ms.RowPitch;
#else
			data = ms.pData;
			pitch = ms.RowPitch;
#endif
			m_lockedSubresource = D3D10CalcSubresource( region.m_startMipLevel, region.m_startFace, m_desc.GetTrueMipCount() );
			return hr;
		}
		else
		{
			const uint32_t w = region.HasBox() ? region.GetWidth() : m_desc.GetMipWidth( region.m_startMipLevel );
			const uint32_t h = region.HasBox() ? region.GetHeight() : m_desc.GetMipHeight( region.m_startMipLevel );
			const uint32_t d = std::max( 1u, region.HasBox() ? region.GetDepth() : m_desc.GetMipDepth( region.m_startMipLevel ) );
			pitch = w * Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() );

			CcpAlignedMallocBuffer buf( "Tr2TextureAL::MapForWriting", pitch * h * d, 32 );
			m_writeStaging.swap( buf );

			if( region.HasBox() )
			{
				m_writeBox = region.m_box;
			}
			else
			{
				m_writeBox = { 0, 0, 0, w, h, d };
			}

			if( m_writeBox.right > m_desc.GetMipWidth( region.m_startMipLevel ) || m_writeBox.bottom > m_desc.GetMipHeight( region.m_startMipLevel ) || m_writeBox.back > m_desc.GetMipDepth( region.m_startMipLevel ) )
			{
				CCP_LOGERR( "TrinityAL: invalid rectangle in Tr2TextureAL::LockWriting" );
				m_writeStaging.clear();
				CCP_ASSERT( false );
				return E_FAIL;
			}

			if( m_writeStaging.empty() )
			{
				return E_OUTOFMEMORY;
			}
#if TRINITY_AL_GUARD_LOCKS
			size_t size;
			if( region.HasBox() )
			{
				size = pitch * region.GetHeight() * region.GetDepth();
			}
			else
			{
				size = pitch * m_desc.GetMipHeight( region.m_startMipLevel ) * m_desc.GetMipDepth( region.m_startMipLevel );
			}

			m_lockGuard.Lock( size, m_writeStaging.get() );
			data = m_lockGuard.GetMemory();
#else
			data = m_writeStaging.get();
#endif

			m_lockedSubresource = D3D10CalcSubresource( region.m_startMipLevel, region.m_startFace, m_desc.GetTrueMipCount() );
			return S_OK;
		}
	}

	void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return;
		}
#if TRINITY_AL_GUARD_LOCKS
		m_lockGuard.Unlock();
#endif
		if( HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			renderContext.m_context->Unmap( m_texture, m_lockedSubresource );
		}
		else
		{
			const uint32_t w = m_writeBox.GetWidth();
			const uint32_t pitch = w * Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() );

			if( renderContext.m_context )
			{
				D3D11_BOX box;
				box.left = m_writeBox.left;
				box.top = m_writeBox.top;
				box.right = m_writeBox.right;
				box.bottom = m_writeBox.bottom;
				box.front = m_writeBox.front;
				box.back = m_writeBox.back;

				auto mip = m_lockedSubresource % m_desc.GetTrueMipCount();

				if( m_writeBox.right > m_desc.GetMipWidth( mip ) || m_writeBox.bottom > m_desc.GetMipHeight( mip ) || m_writeBox.back > m_desc.GetMipDepth( mip ) )
				{
					CCP_LOGERR( "TrinityAL: invalid rectangle in Tr2TextureAL::UnlockWriting" );
					return;
				}

				renderContext.m_context->UpdateSubresource( m_texture, m_lockedSubresource, &box, m_writeStaging.get(), pitch, pitch * m_writeBox.GetHeight() );
			}

			m_writeStaging.clear();
			m_writeBox = { 0, 0, 0, 0, 0, 0 };
		}
	}

	ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext )
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

		auto subResource = D3D10CalcSubresource( region.m_startMipLevel, region.m_startFace, m_desc.GetTrueMipCount() );

		if( region.HasBox() )
		{
			D3D11_BOX box;
			box.left = region.m_box.left;
			box.top = region.m_box.top;
			box.front = region.m_box.front;
			box.right = region.m_box.right;
			box.bottom = region.m_box.bottom;
			box.back = region.m_box.back;

			renderContext.m_context->UpdateSubresource( m_texture, subResource, &box, source, pitch, slicePitch );
		}
		else
		{
			renderContext.m_context->UpdateSubresource( m_texture, subResource, nullptr, source, pitch, slicePitch );
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
			renderContext.m_context->CopyResource( m_texture, source.m_texture );
			return S_OK;
		}

		Tr2TextureSubresource src = sourceSubresource;
		Tr2TextureSubresource dst = destSubresource;

		if( !Crop( src, source.m_desc, dst, m_desc ) )
		{
			return E_FAIL;
		}

		D3D11_BOX box = { src.m_box.left, src.m_box.top, src.m_box.front, src.m_box.right, src.m_box.bottom, src.m_box.back };

		const uint32_t srcMipCount = source.m_desc.GetTrueMipCount();
		const uint32_t dstMipCount = m_desc.GetTrueMipCount();

		const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

		for( uint32_t mip = 0; mip != mipCount; ++mip )
		{
			for( uint32_t face = 0; face != src.GetFaceCount(); ++face )
			{
				renderContext.m_context->CopySubresourceRegion(
					m_texture,
					D3D10CalcSubresource( dst.m_startMipLevel + mip, dst.m_startFace + face, dstMipCount ),
					dst.m_box.left,
					dst.m_box.top,
					dst.m_box.front,
					source.m_texture,
					D3D10CalcSubresource( src.m_startMipLevel + mip, src.m_startFace + face, srcMipCount ),
					&box );
			}

			if( mip + 1 != src.GetMipCount() )
			{
				AdvanceMip( src, source.m_desc, mip );
				AdvanceMip( dst, m_desc, mip );
			}

			box.left = box.left >> 1;
			box.right = std::max( box.right >> 1, box.left + 1 );
			box.top = box.top >> 1;
			box.bottom = std::max( box.bottom >> 1, box.top + 1 );
			box.front = box.front >> 1;
			box.back = std::max( box.back >> 1, box.front + 1 );
		}

		return S_OK;
	}

	ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
	{
		if( !HasFlag( m_gpuUsage, Tr2GpuUsage::RENDER_TARGET ) || !HasFlag( m_gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			return E_INVALIDCALL;
		}
		if( m_desc.GetTrueMipCount() <= 1 )
		{
			return S_OK;
		}
		renderContext.m_context->GenerateMips( m_view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR] );
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

		renderContext.m_context->ResolveSubresource( destination.m_texture, 0, m_texture, 0, static_cast<DXGI_FORMAT>( m_desc.GetFormat() ) );

		return S_OK;
	}

	uintptr_t Tr2TextureAL::GetSharedHandle() const
	{
		IDXGIResource* pOtherResource( NULL );
		if( m_texture == NULL )
		{
			CCP_AL_LOGERR( "GetSharedHandle: texture is NULL" );
			return 0;
		}
		HRESULT HR = m_texture->QueryInterface( __uuidof( IDXGIResource ), (void**)&pOtherResource );
		if( FAILED( HR ) )
		{
			CCP_AL_LOGERR( "GetSharedHandle: QueryInterface failed: 0x%x", HR );
			return 0;
		}
		else
		{
			HANDLE sharedHandle;
			HR = pOtherResource->GetSharedHandle( &sharedHandle );
			if( FAILED( HR ) )
			{
				CCP_AL_LOGERR( "GetSharedHandle: GetSharedHandle failed: 0x%x", HR );
				pOtherResource->Release();
				return 0;
			}
			pOtherResource->Release();
			return reinterpret_cast<uintptr_t>( sharedHandle );
		}
	}

	ALResult Tr2TextureAL::Attach( ID3D11Texture2D* texture, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( texture == nullptr || !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		D3D11_TEXTURE2D_DESC dx11Desc;
		texture->GetDesc( &dx11Desc );

		Tr2BitmapDimensions desc( dx11Desc.Width, dx11Desc.Height, dx11Desc.MipLevels, static_cast<Tr2RenderContextEnum::PixelFormat>( dx11Desc.Format ) );
		Tr2MsaaDesc msaa( dx11Desc.SampleDesc.Count, dx11Desc.SampleDesc.Quality );
		Tr2GpuUsage::Type gpuUsage = Tr2GpuUsage::NONE;
		Tr2CpuUsage::Type cpuUsage = Tr2CpuUsage::NONE;

		cpuUsage = dx11Desc.SampleDesc.Count > 1 ? Tr2CpuUsage::NONE : Tr2CpuUsage::READ;
		if( dx11Desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ )
		{
			cpuUsage = cpuUsage | Tr2CpuUsage::READ;
		}
		if( dx11Desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE )
		{
			if( dx11Desc.Usage == D3D11_USAGE_DYNAMIC )
			{
				cpuUsage = cpuUsage | Tr2CpuUsage::WRITE_OFTEN;
			}
			else
			{
				cpuUsage = cpuUsage | Tr2CpuUsage::WRITE;
			}
		}
		gpuUsage = Tr2GpuUsage::NONE;
		if( dx11Desc.BindFlags & D3D11_BIND_SHADER_RESOURCE )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::SHADER_RESOURCE;
		}
		if( dx11Desc.BindFlags & D3D11_BIND_RENDER_TARGET )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::RENDER_TARGET;
		}
		if( dx11Desc.BindFlags & D3D11_BIND_DEPTH_STENCIL )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::DEPTH_STENCIL;
		}
		if( dx11Desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::UNORDERED_ACCESS;
		}

		FORWARD_HR( CreateViews( texture, desc, msaa, gpuUsage, cpuUsage, true, renderContext ) );

		m_texture = texture;
		m_desc = desc;
		m_msaa = msaa;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;

		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, m_desc, m_msaa );

		return S_OK;
	}

	void Tr2TextureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2TextureAL";

		unsigned size = 0;
		for( unsigned i = 0; i < m_desc.GetTrueMipCount(); ++i )
		{
			size += m_desc.GetMipSize( i );
		}

		description["size"] = std::to_string( size * std::max( 1u, m_desc.GetArraySize() ) * std::max( 1u, m_msaa.samples ) );
		description["width"] = std::to_string( long long( m_desc.GetWidth() ) );
		description["height"] = std::to_string( long long( m_desc.GetHeight() ) );
		description["depth"] = std::to_string( m_desc.GetDepth() );
		description["mipLevels"] = std::to_string( long long( m_desc.GetTrueMipCount() ) );
		description["format"] = std::to_string( long long( m_desc.GetFormat() ) );
		description["texType"] = std::to_string( int( m_desc.GetType() ) );
		description["array"] = std::to_string( m_desc.GetArraySize() );
		description["cpuUsage"] = std::to_string( int( m_cpuUsage ) );
		description["gpuUsage"] = std::to_string( int( m_gpuUsage ) );
		description["msaa"] = std::to_string( m_msaa.samples );
		description["name"] = m_name;
	}

	ALResult Tr2TextureAL::SetName( const char* name )
	{
		m_name = name;
		SetDebugName( m_texture, name );
		for( auto& view : m_view )
		{
			SetDebugName( view, name );
		}
		for( auto& view : m_renderTarget )
		{
			SetDebugName( view, name );
		}
		for( auto& view : m_depthStencil )
		{
			SetDebugName( view, name );
		}
		for( auto& view : m_uav )
		{
			SetDebugName( view, name );
		}
		return S_OK;
	}
}

#endif
