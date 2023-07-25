////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2TextureALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"
#include "ALLog.h"

#pragma warning( push )
#pragma warning( disable : 4189 )

using namespace Tr2RenderContextEnum;

namespace
{
	ALResult CheckCreationFlags( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage )
	{
		if( HasBufferFlags( gpuUsage ) )
		{
			return E_INVALIDARG;
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
		if( HasFlag( cpuUsage, Tr2CpuUsage::READ ) && HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDARG;
		}
		return S_OK;
	}

	D3D12_RESOURCE_DESC TextureDesc( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE )
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Width = desc.GetWidth();
		resourceDesc.Height = desc.GetHeight();
		switch( desc.GetType() )
		{
		case TEX_TYPE_1D:
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			resourceDesc.DepthOrArraySize = UINT16( desc.GetArraySize() );
			break;
		case TEX_TYPE_3D:
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			resourceDesc.DepthOrArraySize = UINT16( desc.GetDepth() );
			break;
		default:
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourceDesc.DepthOrArraySize = UINT16( desc.GetArraySize() );
			break;
		}
		resourceDesc.MipLevels = UINT16( desc.GetTrueMipCount() );
		resourceDesc.Format = DXGI_FORMAT( Tr2RenderContextEnum::MakeTypeless( desc.GetFormat() ) );
		resourceDesc.SampleDesc.Count = msaa.samples;
		resourceDesc.SampleDesc.Quality = msaa.quality;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = flags;
		return resourceDesc;
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
}


namespace
{

	bool FormatIsBGR( DXGI_FORMAT format )
	{
		switch( format )
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}

	bool FormatIsSRGB( DXGI_FORMAT format )
	{
		switch( format )
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}

	DXGI_FORMAT ConvertSRVtoResourceFormat( DXGI_FORMAT format )
	{
		switch( format )
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return DXGI_FORMAT_R32G32B32A32_TYPELESS;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			return DXGI_FORMAT_R32_TYPELESS;
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SINT:
			return DXGI_FORMAT_R16_TYPELESS;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SINT:
			return DXGI_FORMAT_R8_TYPELESS;
		default:
			return format;
		}
	}



} // anonymous namespace


namespace TrinityALImpl
{
	struct Tr2TextureAL::MipMapGenerator
	{
		MipMapGenerator( Tr2PrimaryRenderContextAL& device ) :
			m_device( device )
		{
		}

		~MipMapGenerator()
		{
			if( m_staging )
			{
				RELEASE_LATER( &m_device, m_staging );
			}
			if( m_descriptorHeap )
			{
				RELEASE_LATER( &m_device, m_descriptorHeap );
			}
			if( m_resourceCopy )
			{
				RELEASE_LATER( &m_device, m_resourceCopy );
			}
			if( m_bgrHeap )
			{
				RELEASE_LATER( &m_device, m_bgrHeap );
			}
			if( m_bgrResourceCopy )
			{
				RELEASE_LATER( &m_device, m_bgrResourceCopy );
			}
			if( m_bgrAliasCopy )
			{
				RELEASE_LATER( &m_device, m_bgrAliasCopy );
			}
		}

		ALResult GenerateMips_UnorderedAccessPath( _In_ ID3D12Resource* resource, DXGI_FORMAT format, ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES resourceState )
		{
			const auto desc = resource->GetDesc();
			if( FormatIsBGR( format ) || FormatIsSRGB( format ) )
			{
				return E_INVALIDARG;
			}

			D3D12_RESOURCE_STATES readState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			CComPtr<ID3D12Resource> staging;
			// Create a staging resource if we have to
			if( ( desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ) == 0 )
			{
				if( !m_staging )
				{
					D3D12_HEAP_PROPERTIES defaultHeapProperties = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_DEFAULT );

					D3D12_RESOURCE_DESC stagingDesc = desc;
					stagingDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

					CR_RETURN_HR( m_device.m_device->CreateCommittedResource(
						&defaultHeapProperties,
						D3D12_HEAP_FLAG_NONE,
						&stagingDesc,
						D3D12_RESOURCE_STATE_COPY_DEST,
						nullptr,
						IID_PPV_ARGS( &m_staging ) ) );
					SetDebugName( m_staging, "GenerateMips UAVCopy" );
				}
				else
				{
					auto restore = TrinityALImpl::Transition( m_staging, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST );
					commandList->ResourceBarrier( 1, &restore );
				}
				
				staging = m_staging;

				// Copy the resource to staging
				auto from = TrinityALImpl::Transition( resource, resourceState, D3D12_RESOURCE_STATE_COPY_SOURCE );
				commandList->ResourceBarrier( 1, &from );
				commandList->CopyResource( staging, resource );
				auto to = TrinityALImpl::Transition( staging, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
				commandList->ResourceBarrier( 1, &to );
			}
			else
			{
				// Resource is already a UAV so we can do this in-place
				staging = resource;
				if( ( resourceState & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) == 0 )
				{
					auto barrier = TrinityALImpl::Transition( staging, resourceState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
					commandList->ResourceBarrier( 1, &barrier );
				}
				else
				{
					readState = resourceState;
				}
			}

			uint32_t descriptorSize = m_device.m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
			// Create a descriptor heap that holds our resource descriptors
			if( !m_descriptorHeap )
			{
				D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
				descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				descriptorHeapDesc.NumDescriptors = desc.MipLevels;
				CR_RETURN_HR( m_device.m_device->CreateDescriptorHeap( &descriptorHeapDesc, IID_PPV_ARGS( &m_descriptorHeap ) ) );
				SetDebugName( m_descriptorHeap, "GenerateMips DescriptorHeap" );


				// Create the top-level SRV
				D3D12_CPU_DESCRIPTOR_HANDLE handleIt( m_descriptorHeap->GetCPUDescriptorHandleForHeapStart() );
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = format;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				if( desc.DepthOrArraySize > 1 )
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
					srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
				}
				else
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Texture2D.MostDetailedMip = 0;
					srvDesc.Texture2D.MipLevels = desc.MipLevels;
				}

				m_device.m_device->CreateShaderResourceView( staging, &srvDesc, handleIt );

				// Create the UAVs for the tail
				for( uint16_t mip = 1; mip < desc.MipLevels; ++mip )
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
					uavDesc.Format = format;
					if( desc.DepthOrArraySize > 1 )
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						uavDesc.Texture2DArray.MipSlice = mip;
						uavDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
					}
					else
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						uavDesc.Texture2D.MipSlice = mip;
					}

					handleIt.ptr += descriptorSize;
					m_device.m_device->CreateUnorderedAccessView( staging, nullptr, &uavDesc, handleIt );
				}
			}

			// Set up UAV barrier (used in loop)
			D3D12_RESOURCE_BARRIER barrierUAV = {};
			barrierUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrierUAV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrierUAV.UAV.pResource = staging;

			// Barrier for transitioning the subresources to UAVs
			D3D12_RESOURCE_BARRIER srv2uavDesc = {};
			srv2uavDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			srv2uavDesc.Transition.pResource = staging;
			srv2uavDesc.Transition.Subresource = 0;
			srv2uavDesc.Transition.StateBefore = readState;
			srv2uavDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

			// Barrier for transitioning the subresources to SRVs
			D3D12_RESOURCE_BARRIER uav2srvDesc = {};
			uav2srvDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			uav2srvDesc.Transition.pResource = staging;
			uav2srvDesc.Transition.Subresource = 0;
			uav2srvDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			uav2srvDesc.Transition.StateAfter = readState;

			// Set up state
			commandList->SetComputeRootSignature( m_device.m_genMipsResources->rootSignature );
			commandList->SetPipelineState( desc.DepthOrArraySize > 1 ? m_device.m_genMipsResources->generateMipsArrayPSO : m_device.m_genMipsResources->generateMipsPSO );
			commandList->SetDescriptorHeaps( 1, &m_descriptorHeap );
			commandList->SetComputeRootDescriptorTable( TrinityALImpl::GenerateMipsResources::SourceTexture, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart() );

			// Get the descriptor handle -- uavH will increment over each loop
			D3D12_GPU_DESCRIPTOR_HANDLE uavH = { m_descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + descriptorSize }; // offset by 1 descriptor

			// Process each mip
			auto mipWidth = static_cast<uint32_t>( desc.Width );
			uint32_t mipHeight = desc.Height;
			for( uint32_t mip = 1; mip < desc.MipLevels; ++mip )
			{
				mipWidth = std::max<uint32_t>( 1, mipWidth >> 1 );
				mipHeight = std::max<uint32_t>( 1, mipHeight >> 1 );

				// Transition the mip to a UAV
				srv2uavDesc.Transition.Subresource = mip;
				commandList->ResourceBarrier( 1, &srv2uavDesc );

				// Bind the mip subresources
				commandList->SetComputeRootDescriptorTable( TrinityALImpl::GenerateMipsResources::TargetTexture, uavH );

				// Set constants
				TrinityALImpl::GenerateMipsResources::ConstantData constants;
				constants.SrcMipIndex = mip - 1;
				constants.InvOutTexelSizeX = 1 / float( mipWidth );
				constants.InvOutTexelSizeY = 1 / float( mipHeight );
				commandList->SetComputeRoot32BitConstants(
					TrinityALImpl::GenerateMipsResources::Constants,
					TrinityALImpl::GenerateMipsResources::Num32BitConstants,
					&constants,
					0 );

				// Process this mip
				commandList->Dispatch(
					( mipWidth + TrinityALImpl::GenerateMipsResources::ThreadGroupSize - 1 ) / TrinityALImpl::GenerateMipsResources::ThreadGroupSize,
					( mipHeight + TrinityALImpl::GenerateMipsResources::ThreadGroupSize - 1 ) / TrinityALImpl::GenerateMipsResources::ThreadGroupSize,
					desc.DepthOrArraySize );

				commandList->ResourceBarrier( 1, &barrierUAV );

				// Transition the mip to an SRV
				uav2srvDesc.Transition.Subresource = mip;
				commandList->ResourceBarrier( 1, &uav2srvDesc );

				// Offset the descriptor heap handles
				uavH.ptr += descriptorSize;
			}

			// If the staging resource is NOT the same as the resource, we need to copy everything back
			if( staging != resource )
			{
				// Transition the resources ready for copy
				D3D12_RESOURCE_BARRIER barriers[2] = {
					TrinityALImpl::Transition( staging, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE ),
					TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST )
				};

				commandList->ResourceBarrier( 2, barriers );
				// Copy the entire resource back
				commandList->CopyResource( resource, staging );

				// Transition the target resource back to pixel shader resource
				auto barrier = TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_COPY_DEST, resourceState );
				commandList->ResourceBarrier( 1, &barrier );
			}
			else if( ( resourceState & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) == 0 )
			{
				auto barrier = TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, resourceState );
				commandList->ResourceBarrier( 1, &barrier );
			}

			m_device.DirtyDescriptorCache();

			return S_OK;
		}

		ALResult GenerateMips_TexturePath( _In_ ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES resourceState )
		{
			const auto resourceDesc = resource->GetDesc();
			if( FormatIsBGR( resourceDesc.Format ) && !FormatIsSRGB( resourceDesc.Format ) )
			{
				return E_FAIL;
			}

			if( !m_resourceCopy )
			{
				auto copyDesc = resourceDesc;
				copyDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
				copyDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

				auto heapProperties = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_DEFAULT );

				// Create a resource with the same description, but without SRGB, and with UAV flags
				CR_RETURN_HR( m_device.m_device->CreateCommittedResource(
					&heapProperties,
					D3D12_HEAP_FLAG_NONE,
					&copyDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS( &m_resourceCopy ) ) );
			}
			else
			{
				auto restore = TrinityALImpl::Transition( m_resourceCopy, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST );
				commandList->ResourceBarrier( 1, &restore );
			}

			// Copy the resource data
			auto barrier = TrinityALImpl::Transition( resource, resourceState, D3D12_RESOURCE_STATE_COPY_SOURCE );
			commandList->ResourceBarrier( 1, &barrier );
			commandList->CopyResource( m_resourceCopy, resource );
			barrier = TrinityALImpl::Transition( m_resourceCopy, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
			commandList->ResourceBarrier( 1, &barrier );

			// Generate the mips
			GenerateMips_UnorderedAccessPath( m_resourceCopy, DXGI_FORMAT_R8G8B8A8_UNORM, commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );

			// Direct copy back
			barrier = TrinityALImpl::Transition( m_resourceCopy, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE );
			commandList->ResourceBarrier( 1, &barrier );
			barrier = TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST );
			commandList->ResourceBarrier( 1, &barrier );
			commandList->CopyResource( resource, m_resourceCopy );
			barrier = TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_COPY_DEST, resourceState );
			commandList->ResourceBarrier( 1, &barrier );
			return S_OK;
		}

		ALResult GenerateMips_TexturePathBGR( _In_ ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES resourceState )
		{
			const auto resourceDesc = resource->GetDesc();
			if( !FormatIsBGR( resourceDesc.Format ) )
			{
				return E_FAIL;
			}

			if( !m_bgrHeap )
			{
				// Create a resource with the same description, but without SRGB, and with UAV flags
				auto copyDesc = resourceDesc;
				copyDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
				copyDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

				D3D12_HEAP_DESC heapDesc = {};
				auto allocInfo = m_device.m_device->GetResourceAllocationInfo( 0, 1, &resourceDesc );
				heapDesc.SizeInBytes = allocInfo.SizeInBytes;
				heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
				heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

				CR_RETURN_HR( m_device.m_device->CreateHeap( &heapDesc, IID_PPV_ARGS( &m_bgrHeap ) ) );

				CR_RETURN_HR( m_device.m_device->CreatePlacedResource(
					m_bgrHeap,
					0,
					&copyDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS( &m_bgrResourceCopy ) ) );

				// Create a BGRA alias
				auto aliasDesc = resourceDesc;
				aliasDesc.Format = ( resourceDesc.Format == DXGI_FORMAT_B8G8R8X8_TYPELESS ) ? DXGI_FORMAT_B8G8R8X8_TYPELESS : DXGI_FORMAT_B8G8R8A8_TYPELESS;
				aliasDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

				CR_RETURN_HR( m_device.m_device->CreatePlacedResource(
					m_bgrHeap,
					0,
					&aliasDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS( &m_bgrAliasCopy ) ) );
			}
			else
			{
				auto restore = TrinityALImpl::Transition( m_bgrAliasCopy, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST );
				commandList->ResourceBarrier( 1, &restore );
			}

			// Copy the resource data
			auto barrier = TrinityALImpl::AliasBarrier( nullptr, m_bgrAliasCopy );
			commandList->ResourceBarrier( 1, &barrier );
			barrier = TrinityALImpl::Transition( resource, resourceState, D3D12_RESOURCE_STATE_COPY_SOURCE );
			commandList->ResourceBarrier( 1, &barrier );
			commandList->CopyResource( m_bgrAliasCopy, resource );

			// Generate the mips
			barrier = TrinityALImpl::AliasBarrier( m_bgrAliasCopy, m_bgrResourceCopy );
			commandList->ResourceBarrier( 1, &barrier );
			barrier = TrinityALImpl::Transition( m_bgrResourceCopy, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
			commandList->ResourceBarrier( 1, &barrier );
			FORWARD_HR( GenerateMips_UnorderedAccessPath( m_bgrResourceCopy, DXGI_FORMAT_R8G8B8A8_UNORM, commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) );

			// Direct copy back
			barrier = TrinityALImpl::AliasBarrier( m_bgrResourceCopy, m_bgrAliasCopy );
			commandList->ResourceBarrier( 1, &barrier );
			barrier = TrinityALImpl::Transition( m_bgrAliasCopy, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE );
			commandList->ResourceBarrier( 1, &barrier );
			barrier = TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST );
			commandList->ResourceBarrier( 1, &barrier );
			commandList->CopyResource( resource, m_bgrAliasCopy );
			barrier = TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_COPY_DEST, resourceState );
			commandList->ResourceBarrier( 1, &barrier );
			return S_OK;
		}

		
		CComPtr<ID3D12Resource> m_staging;
		CComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
		CComPtr<ID3D12Resource> m_resourceCopy;
		CComPtr<ID3D12Heap> m_bgrHeap;
		CComPtr<ID3D12Resource> m_bgrResourceCopy;
		CComPtr<ID3D12Resource> m_bgrAliasCopy;
		Tr2PrimaryRenderContextAL& m_device;
	};

	Tr2TextureAL::Tr2TextureAL()
		:m_owner( nullptr ),
		m_currentTextureIndex( 0 ),
		m_defaultState( D3D12_RESOURCE_STATE_COMMON ),
		m_gpuUsage( Tr2GpuUsage::NONE ),
		m_cpuUsage( Tr2CpuUsage::NONE ),
		m_mipMapGenerator( nullptr )
	{
		m_mappedScratch = m_writeScratches.end();
	}

	Tr2TextureAL::~Tr2TextureAL()
	{
		Destroy();
	}

	ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		FORWARD_HR( CheckCreationFlags( desc, msaa, gpuUsage, cpuUsage ) );

		if( !IsWritable( gpuUsage ) && !HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
		{
			return E_INVALIDARG;
		}

		if( desc.GetType() == TEX_TYPE_INVALID )
		{
			return E_INVALIDARG;
		}

		if( desc.GetFormat() == Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
		{
			return E_INVALIDARG;
		}

		if( desc.GetWidth() == 0 || desc.GetHeight() == 0 || ( desc.GetType() == TEX_TYPE_3D && desc.GetDepth() == 0 ) || ( desc.GetType() != TEX_TYPE_3D && desc.GetArraySize() == 0 ) )
		{
			return E_INVALIDARG;
		}

		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if( !HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
			{
				resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		D3D12_RESOURCE_STATES defaultState;
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			defaultState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			defaultState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			defaultState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			defaultState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		else
		{
			defaultState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		D3D12_RESOURCE_STATES creationState = defaultState;
		if( initialData )
		{
			creationState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		CComPtr<ID3D12Resource> texture;
		auto heap = HeapDesc( D3D12_HEAP_TYPE_DEFAULT );
		auto resourceDesc = TextureDesc( desc, msaa, resourceFlags );
		CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			creationState,
			nullptr,
			IID_PPV_ARGS( &texture ) ) );

		CComPtr<ID3D12Resource> scratch;
		uint64_t writeScratchSize = 0;
		if( initialData )
		{
			auto subresources = std::max( 1u, desc.GetArraySize() ) * resourceDesc.MipLevels;
			writeScratchSize = GetRequiredIntermediateSize( texture, 0, subresources );

			auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
			auto scratchDesc = BufferDesc( writeScratchSize );
			CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
				&scratchHeap,
				D3D12_HEAP_FLAG_NONE,
				&scratchDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS( &scratch ) ) );

			std::vector<D3D12_SUBRESOURCE_DATA> dx12InitialData( desc.GetArraySize() * resourceDesc.MipLevels );
			for( size_t i = 0; i < dx12InitialData.size(); ++i )
			{
				dx12InitialData[i].pData = initialData[i].m_sysMem;
				dx12InitialData[i].RowPitch = initialData[i].m_sysMemPitch;
				dx12InitialData[i].SlicePitch = initialData[i].m_sysMemSlicePitch;
			}


			if( !UpdateSubresources(
				renderContext.m_commandList,
				texture,
				scratch,
				0,
				0,
				subresources,
				&dx12InitialData[0] ) )
			{
				return E_FAIL;
			}
			if( defaultState != creationState )
			{
				renderContext.ResourceBarrierDx12( Transition( texture, creationState, defaultState ) );
			}
			RELEASE_LATER( &renderContext, scratch );
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = GetSrvFormat( desc.GetFormat() );
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			switch( desc.GetType() )
			{
			case TEX_TYPE_1D:
				if( resourceDesc.DepthOrArraySize > 1 )
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					srvDesc.Texture1DArray.MipLevels = resourceDesc.MipLevels;
					srvDesc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
				}
				else
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					srvDesc.Texture1D.MipLevels = resourceDesc.MipLevels;
				}
				break;
			case TEX_TYPE_3D:
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MipLevels = resourceDesc.MipLevels;
				break;
			case TEX_TYPE_CUBE:
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MipLevels = resourceDesc.MipLevels;
				break;
			default:
				if( msaa.samples > 1 )
				{
					if( resourceDesc.DepthOrArraySize > 1 )
					{
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
						srvDesc.Texture2DMSArray.FirstArraySlice = 0;
						srvDesc.Texture2DMSArray.ArraySize = resourceDesc.DepthOrArraySize;
					}
					else
					{
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					}
				}
				else
				{
					if( resourceDesc.DepthOrArraySize > 1 )
					{
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						srvDesc.Texture2DArray.MipLevels = resourceDesc.MipLevels;
						srvDesc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
					}
					else
					{
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
					}
				}
			}

			m_srvDesc[0] = srvDesc;
			m_srvDesc[1] = srvDesc;
			m_srvDesc[1].Format = GetSrvFormat( MakeSrgb( desc.GetFormat() ) );

			renderContext.CreateShaderResourceView(texture, m_srvDesc[0], m_view[0]);
			renderContext.CreateShaderResourceView(texture, m_srvDesc[1], m_view[1]);
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NumDescriptors = resourceDesc.MipLevels;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

			D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
			uav.Format = GetSrvFormat( desc.GetFormat() );
			for( uint32_t i = 0; i < resourceDesc.MipLevels; ++i )
			{
				switch( desc.GetType() )
				{
				case TEX_TYPE_1D:
					if( resourceDesc.DepthOrArraySize > 1 )
					{
						uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
						uav.Texture1DArray.MipSlice = i;
						uav.Texture1DArray.FirstArraySlice = 0;
						uav.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
					}
					else
					{
						uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
						uav.Texture1D.MipSlice = i;
					}
					break;
				case TEX_TYPE_3D:
					uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
					uav.Texture3D.MipSlice = i;
					uav.Texture3D.FirstWSlice = 0;
					uav.Texture3D.WSize = UINT( std::max( 1, resourceDesc.DepthOrArraySize >> i ) );
					break;
				case TEX_TYPE_CUBE:
					uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					uav.Texture2DArray.ArraySize = 6;
					uav.Texture2DArray.FirstArraySlice = 0;
					uav.Texture2DArray.PlaneSlice = 0;
					uav.Texture2DArray.MipSlice = i;
					break;
				default:
					if( resourceDesc.DepthOrArraySize > 1 )
					{
						uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						uav.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
						uav.Texture2DArray.FirstArraySlice = 0;
						uav.Texture2DArray.PlaneSlice = 0;
						uav.Texture2DArray.MipSlice = i;
					}
					else
					{
						uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						uav.Texture2D.PlaneSlice = 0;
						uav.Texture2D.MipSlice = i;
					}
				}

				std::shared_ptr<UnorderedAccessViewDx12> uavView;
				renderContext.CreateUnorderedAccessView(texture, nullptr, uav, uavView);
				m_uav.push_back(uavView);
			}
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtv;
			rtv.Format = DXGI_FORMAT( desc.GetFormat() );
			if( desc.GetType() == TEX_TYPE_3D )
			{
				rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				rtv.Texture3D.MipSlice = 0;
				rtv.Texture3D.WSize = 1;
				m_rtv.resize( 2 * desc.GetDepth() );
				auto srgb = DXGI_FORMAT( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );

				for( uint32_t i = 0; i < desc.GetDepth(); ++i )
				{
					rtv.Texture3D.FirstWSlice = i;
					rtv.Format = DXGI_FORMAT( desc.GetFormat() );
					renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[i * 2] );
					if( srgb != rtv.Format )
					{
						rtv.Format = srgb;
						renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[i * 2 + 1] );
					}
					else
					{
						m_rtv[i * 2 + 1] = m_rtv[i * 2];
					}
				}
			}
			else if( desc.GetArraySize() > 1 )
			{
				if( msaa.samples > 1 )
				{
					rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
					rtv.Texture2DMSArray.ArraySize = 1;

					m_rtv.resize( 2 * desc.GetArraySize() );
					auto srgb = DXGI_FORMAT( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );

					for( uint32_t i = 0; i < desc.GetArraySize(); ++i )
					{
						rtv.Texture2DMSArray.FirstArraySlice = i;
						rtv.Format = DXGI_FORMAT( desc.GetFormat() );
						renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[i * 2] );
						if( srgb != rtv.Format )
						{
							rtv.Format = srgb;
							renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[i * 2 + 1] );
						}
						else
						{
							m_rtv[i * 2 + 1] = m_rtv[i * 2];
						}
					}
				}
				else
				{
					rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					rtv.Texture2DArray.ArraySize = 1;
					rtv.Texture2DArray.MipSlice = 0;
					rtv.Texture2DArray.PlaneSlice = 0;

					m_rtv.resize( 2 * desc.GetArraySize() );
					auto srgb = DXGI_FORMAT( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );

					for( uint32_t i = 0; i < desc.GetArraySize(); ++i )
					{
						rtv.Texture2DArray.FirstArraySlice = i;
						rtv.Format = DXGI_FORMAT( desc.GetFormat() );
						renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[i * 2] );
						if( srgb != rtv.Format )
						{
							rtv.Format = srgb;
							renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[i * 2 + 1] );
						}
						else
						{
							m_rtv[i * 2 + 1] = m_rtv[i * 2];
						}
					}
				}
			}
			else
			{
				if( msaa.samples > 1 )
				{
					rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					rtv.Texture2D.MipSlice = 0;
					rtv.Texture2D.PlaneSlice = 0;
				}
				m_rtv.resize( 2 );
				renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[0] );

				auto srgb = DXGI_FORMAT( Tr2RenderContextEnum::MakeSrgb( desc.GetFormat() ) );
				if( srgb != rtv.Format )
				{
					rtv.Format = srgb;
					renderContext.CreateRenderTargetView( texture, &rtv, m_rtv[1] );
				}
				else
				{
					m_rtv[1] = m_rtv[0];
				}
			}
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = 1;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

			D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
			dsv.Format = DXGI_FORMAT( desc.GetFormat() );
			dsv.Flags = D3D12_DSV_FLAG_NONE;
			if( msaa.samples > 1 )
			{
				dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsv.Texture2D.MipSlice = 0;
			}

			renderContext.CreateDepthStencilView(texture, dsv, m_dsv);
		}

		m_textures.push_back( texture );
		m_desc = desc;
		m_msaa = msaa;
		m_owner = &renderContext;
		m_defaultState = defaultState;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;
		if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) && scratch )
		{
			WriteScratch s = { scratch, writeScratchSize, renderContext.GetCurrentFrameIndexDx12() };
			m_writeScratches.push_back( s );
		}

		return S_OK;
	}

	void Tr2TextureAL::Destroy()
	{
		m_mipMapGenerator.reset();

		for( auto it = begin( m_textures ); it != end( m_textures ); ++it )
		{
			RELEASE_LATER( m_owner, *it );
		}
		m_textures.clear();
		for( auto it = begin( m_writeScratches ); it != end( m_writeScratches ); ++it )
		{
			RELEASE_LATER( m_owner, it->scratch );
		}
		m_writeScratches.clear();
		m_mappedScratch = m_writeScratches.end();
		m_readScratch = nullptr;
		m_currentTextureIndex = 0;
		memset( &m_desc, 0, sizeof( m_desc ) );
		m_msaa = Tr2MsaaDesc();
		m_owner = nullptr;
		m_defaultState = D3D12_RESOURCE_STATE_COMMON;
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_cpuUsage = Tr2CpuUsage::NONE;

		// JB: Don't need to ReleaseLater() because views aren't actually GPU visible
		m_view[Tr2RenderContextEnum::COLOR_SPACE_LINEAR] = nullptr;
		m_view[Tr2RenderContextEnum::COLOR_SPACE_SRGB] = nullptr;
		m_rtv.clear();
		m_dsv = nullptr;
		m_uav.clear();
	}

	bool Tr2TextureAL::IsValid() const
	{
		return !m_textures.empty();
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
		if( !IsWritable( destination.m_gpuUsage ) )
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

		auto srcState = m_defaultState;
		renderContext.IsBoundDx12( *this, srcState );

		D3D12_RESOURCE_BARRIER barriers[2];
		ID3D12Resource* resources[2];
		uint32_t barrierCount = 0;
		if( srcState != D3D12_RESOURCE_STATE_RESOLVE_SOURCE )
		{
			resources[barrierCount] = GetResourceDx12();
			barriers[barrierCount++] = Transition( GetResourceDx12(), srcState, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, 0 );
		}
		if( destination.m_defaultState != D3D12_RESOURCE_STATE_RESOLVE_DEST )
		{
			resources[barrierCount] = destination.GetResourceDx12();
			barriers[barrierCount++] = Transition( destination.GetResourceDx12(), destination.m_defaultState, D3D12_RESOURCE_STATE_RESOLVE_DEST, 0 );
		}
		if( barrierCount )
		{
			renderContext.ResourceBarrierDx12( barrierCount, barriers );
			renderContext.FlushBarriersDx12( barrierCount, resources );
		}

		renderContext.m_commandList->ResolveSubresource( destination.GetResourceDx12(), 0, GetResourceDx12(), 0, DXGI_FORMAT( m_desc.GetFormat() ) );

		if( barrierCount )
		{
			for( uint32_t i = 0; i < barrierCount; ++i )
			{
				std::swap( barriers[i].Transition.StateAfter, barriers[i].Transition.StateBefore );
			}
			renderContext.ResourceBarrierDx12( barrierCount, barriers );
		}
		return S_OK;
	}

	ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetTrueMipCount() == 1 )
		{
			return S_OK;
		}

		const auto desc = m_textures[0]->GetDesc();
		if( m_desc.GetType() != TEX_TYPE_2D && m_desc.GetType() != TEX_TYPE_CUBE )
		{
			return E_INVALIDCALL;
		}

		renderContext.SetResourceSet( ::Tr2ResourceSetAL() );
		renderContext.FlushBarriersDx12( m_textures[0] );

		renderContext.m_dirtyPso = true;

		if( !m_mipMapGenerator )
		{
			m_mipMapGenerator.reset( new MipMapGenerator( *m_owner ) );
		}
		if( m_owner->FormatIsUAVCompatibleDx12( DXGI_FORMAT( m_desc.GetFormat() ) ) )
		{
			FORWARD_HR( m_mipMapGenerator->GenerateMips_UnorderedAccessPath( m_textures[0], DXGI_FORMAT( m_desc.GetFormat() ), renderContext.m_commandList, m_defaultState ) );
		}
		else if( FormatIsBGR( desc.Format ) )
		{
			FORWARD_HR( m_mipMapGenerator->GenerateMips_TexturePathBGR( m_textures[0], renderContext.m_commandList, m_defaultState ) );
		}
		else
		{
			FORWARD_HR( m_mipMapGenerator->GenerateMips_TexturePath( m_textures[0], renderContext.m_commandList, m_defaultState ) );
		}

		return S_OK;
	}

	ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !source.IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		auto dstResource = GetResourceDx12();
		auto srcResource = source.GetResourceDx12();

		auto srcState = source.m_defaultState;
		renderContext.IsBoundDx12( source, srcState );

		if( destSubresource.IsSubresourceFull( m_desc ) && sourceSubresource.IsSubresourceFull( source.m_desc ) )
		{
			{
				D3D12_RESOURCE_BARRIER barriers[2] = {
					TrinityALImpl::Transition( srcResource, srcState, D3D12_RESOURCE_STATE_COPY_SOURCE ),
					TrinityALImpl::Transition( dstResource, m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST ),
				};
				renderContext.ResourceBarrierDx12( 2, barriers );
				renderContext.FlushBarriersDx12( srcResource, dstResource );
			}
			renderContext.m_commandList->CopyResource( dstResource, srcResource );
			{
				D3D12_RESOURCE_BARRIER barriers[2] = {
					TrinityALImpl::Transition( srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE, srcState ),
					TrinityALImpl::Transition( dstResource, D3D12_RESOURCE_STATE_COPY_DEST, m_defaultState ),
				};
				renderContext.ResourceBarrierDx12( 2, barriers );
			}
			return S_OK;
		}


		Tr2TextureSubresource src = sourceSubresource;
		Tr2TextureSubresource dst = destSubresource;

		if( !Crop( src, source.m_desc, dst, m_desc ) )
		{
			return E_FAIL;
		}

		D3D12_BOX box = { src.m_box.left, src.m_box.top, src.m_box.front, src.m_box.right, src.m_box.bottom, src.m_box.back };

		const uint32_t srcMipCount = source.m_desc.GetTrueMipCount();
		const uint32_t dstMipCount = m_desc.GetTrueMipCount();

		const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

		{
			D3D12_RESOURCE_BARRIER barriers[2] = {
				TrinityALImpl::Transition( srcResource, srcState, D3D12_RESOURCE_STATE_COPY_SOURCE ),
				TrinityALImpl::Transition( dstResource, m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST ),
			};
			renderContext.ResourceBarrierDx12( 2, barriers );
			renderContext.FlushBarriersDx12( srcResource, dstResource );
		}

		for( uint32_t mip = 0; mip != mipCount; ++mip )
		{
			for( uint32_t face = 0; face != src.GetFaceCount(); ++face )
			{
				D3D12_TEXTURE_COPY_LOCATION dstLoc = { dstResource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dst.m_startMipLevel + mip + ( dst.m_startFace + face ) * dstMipCount };
				D3D12_TEXTURE_COPY_LOCATION srcLoc = { srcResource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, src.m_startMipLevel + mip + ( src.m_startFace + face ) * srcMipCount };
				renderContext.m_commandList->CopyTextureRegion( &dstLoc, dst.m_box.left, dst.m_box.top, dst.m_box.front, &srcLoc, &box );
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

		{
			D3D12_RESOURCE_BARRIER barriers[2] = {
				TrinityALImpl::Transition( srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE, srcState ),
				TrinityALImpl::Transition( dstResource, D3D12_RESOURCE_STATE_COPY_DEST, m_defaultState ),
			};
			renderContext.ResourceBarrierDx12( 2, barriers );
		}

		return S_OK;
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

	void Tr2TextureAL::GetRegionSize( const Tr2TextureSubresource& region, uint32_t& pitch, uint64_t& size )
	{
		if( region.HasBox() )
		{
			pitch = region.GetWidth() * Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() );
			pitch = ( ( pitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1 ) / D3D12_TEXTURE_DATA_PITCH_ALIGNMENT ) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
			size = region.GetHeight() * region.GetDepth() * pitch;
		}
		else
		{
			pitch = m_desc.GetMipPitch( region.m_startMipLevel );
			pitch = ( ( pitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1 ) / D3D12_TEXTURE_DATA_PITCH_ALIGNMENT ) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
			size = m_desc.GetMipHeight( region.m_startMipLevel ) * m_desc.GetMipDepth( region.m_startMipLevel ) * pitch;
		}
	}

	ALResult Tr2TextureAL::MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() || !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
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

		uint64_t requiredSize = 0;
		uint32_t myPitch = 0;
		GetRegionSize( region, myPitch, requiredSize );

		auto completed = m_owner->GetCompletedFrameIndexDx12();
		m_mappedScratch = m_writeScratches.end();
		for( auto it = begin( m_writeScratches ); it != end( m_writeScratches ); ++it )
		{
			if( completed >= it->frameIndex && requiredSize <= it->size )
			{
				m_mappedScratch = it;
				break;
			}
		}

		if( m_mappedScratch == m_writeScratches.end() )
		{
			auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
			auto scratchDesc = BufferDesc( requiredSize );
			WriteScratch scratch;
			CR_RETURN_HR( m_owner->m_device->CreateCommittedResource(
				&scratchHeap,
				D3D12_HEAP_FLAG_NONE,
				&scratchDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS( &scratch.scratch ) ) );
			scratch.size = requiredSize;
			scratch.frameIndex = m_owner->GetCurrentFrameIndexDx12();
			m_writeScratches.push_back( scratch );
			m_mappedScratch = m_writeScratches.begin() + ( m_writeScratches.size() - 1 );
		}

		D3D12_RANGE range = { 0, 0 };
		CR_RETURN_HR( m_mappedScratch->scratch->Map( 0, &range, &data ) );
		pitch = myPitch;
		m_mappedRegion = region;

		return S_OK;
	}

	void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		if( m_mappedScratch == m_writeScratches.end() || !renderContext.IsValid() || !m_mappedRegion.IsValid() )
		{
			return;
		}

		uint64_t requiredSize = 0;
		uint32_t pitch = 0;
		GetRegionSize( m_mappedRegion, pitch, requiredSize );

		m_mappedScratch->scratch->Unmap( 0, nullptr );

		auto texture = GetResourceDx12();
		auto subresource = m_mappedRegion.m_startFace * m_desc.GetTrueMipCount() + m_mappedRegion.m_startMipLevel;
		D3D12_RESOURCE_DESC desc = texture->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		layout.Offset = 0;
		if( m_mappedRegion.HasBox() )
		{
			D3D12_SUBRESOURCE_FOOTPRINT f = { desc.Format, m_mappedRegion.GetWidth(), m_mappedRegion.GetHeight(), m_mappedRegion.GetDepth(), pitch };
			layout.Footprint = f;
		}
		else
		{
			auto mip = m_mappedRegion.m_startMipLevel;
			D3D12_SUBRESOURCE_FOOTPRINT f = { desc.Format, m_desc.GetMipWidth( mip ), m_desc.GetMipHeight( mip ), m_desc.GetMipDepth( mip ), pitch };
			layout.Footprint = f;
		}
		D3D12_TEXTURE_COPY_LOCATION Dst = { texture, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, subresource };
		D3D12_TEXTURE_COPY_LOCATION Src = { m_mappedScratch->scratch, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, layout };

		renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( texture, m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST ) );
		renderContext.FlushBarriersDx12( texture );

		if( m_mappedRegion.HasBox() )
		{
			D3D12_BOX box = { 0, 0, 0, m_mappedRegion.m_box.GetWidth(), m_mappedRegion.m_box.GetHeight(), m_mappedRegion.m_box.GetDepth() };
			renderContext.m_commandList->CopyTextureRegion( 
				&Dst, 
				m_mappedRegion.m_box.left, 
				m_mappedRegion.m_box.top, 
				m_mappedRegion.m_box.front, 
				&Src, 
				&box );
		}
		else
		{
			renderContext.m_commandList->CopyTextureRegion( &Dst, 0, 0, 0, &Src, nullptr );
		}

		renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( texture, D3D12_RESOURCE_STATE_COPY_DEST, m_defaultState ) );

		m_mappedScratch->frameIndex = m_owner->GetCurrentFrameIndexDx12();

		m_mappedRegion = Tr2TextureSubresource();
		m_mappedScratch = m_writeScratches.end();
	}

	ALResult Tr2TextureAL::MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}

		auto texture = GetResourceDx12();

		CComPtr<ID3D12Resource> scratch = m_readScratch;
		if( !scratch )
		{
			auto totalSize = GetRequiredIntermediateSize( texture, 0, 1 );
			auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_READBACK );
			auto scratchDesc = BufferDesc( totalSize );
			CR_RETURN_HR( m_owner->m_device->CreateCommittedResource(
				&scratchHeap,
				D3D12_HEAP_FLAG_NONE,
				&scratchDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS( &scratch ) ) );
		}

		renderContext.ResourceBarrierDx12( Transition( texture, m_defaultState, D3D12_RESOURCE_STATE_COPY_SOURCE ) );
		renderContext.FlushBarriersDx12( texture );

		auto subresource = region.m_startFace * m_desc.GetTrueMipCount() + region.m_startMipLevel;
		D3D12_RESOURCE_DESC desc = texture->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		m_owner->m_device->GetCopyableFootprints( &desc, subresource, 1, 0, &layout, nullptr, nullptr, nullptr );
		layout.Offset = 0;
		D3D12_TEXTURE_COPY_LOCATION Src = { texture, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, subresource };
		D3D12_TEXTURE_COPY_LOCATION Dst = { scratch, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, layout };

		renderContext.m_commandList->CopyTextureRegion( &Dst, 0, 0, 0, &Src, nullptr );

		renderContext.ResourceBarrierDx12( Transition( texture, D3D12_RESOURCE_STATE_COPY_SOURCE, m_defaultState ) );

		auto hr = renderContext.FlushAndSyncDx12();
		if( FAILED( hr ) )
		{
			RELEASE_LATER( m_owner, scratch );
			scratch = nullptr;
			return hr;
		}

		CR_RETURN_HR( scratch->Map( 0, nullptr, (void**)&data ) );
		m_readScratch = scratch;

		pitch = layout.Footprint.RowPitch;

		return S_OK;
	}

	void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& )
	{
		if( !m_readScratch )
		{
			return;
		}
		D3D12_RANGE writtenRange = { 0, 0 };
		m_readScratch->Unmap( 0, &writtenRange );
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) )
		{
			m_readScratch = nullptr;
		}
	}

	ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t depthPitch, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( m_gpuUsage ) )
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

		void* dest;
		uint32_t destPitch;
		FORWARD_HR( MapForWriting( region, dest, destPitch, renderContext ) );

		auto clamped = region;
		clamped.ClampToTexture( m_desc );

		auto bpp = Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() );
		auto width = clamped.m_box.GetWidth();
		width *= bpp;
		auto depthSlice = static_cast<const uint8_t*>( source );

		for( uint32_t j = clamped.m_box.front; j != clamped.m_box.back; ++j )
		{
			auto src = depthSlice;
			for( uint32_t i = clamped.m_box.top; i != clamped.m_box.bottom; ++i )
			{
				memcpy( dest, src, width );
				dest = static_cast<uint8_t*>( dest ) + destPitch;
				src += pitch;
			}
			depthSlice += depthPitch;
		}
		UnmapForWriting( renderContext );

		return S_OK;
	}



	ID3D12Resource* Tr2TextureAL::GetResourceDx12() const
	{
		if( m_currentTextureIndex < m_textures.size() )
		{
			return m_textures[m_currentTextureIndex];
		}
		return nullptr;
	}

	const std::shared_ptr<RenderTargetViewDx12>& Tr2TextureAL::GetRtvDescriptorHandleDx12( Tr2RenderContextEnum::ColorSpace colorSpace, uint32_t slice ) const
	{
		auto slices = std::max( 1u, m_desc.GetDepth() );
		auto index = m_currentTextureIndex * slices * 2 + 2 * slice + colorSpace;
		return m_rtv[index];
	}

	void Tr2TextureAL::AssignFromSwapChainDx12( const std::vector<CComPtr<ID3D12Resource>>& backBuffers, const std::vector<std::shared_ptr<RenderTargetViewDx12>>& rtvs, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();
		if( !backBuffers.empty() )
		{
			auto desc = backBuffers[0]->GetDesc();
			m_msaa.samples = desc.SampleDesc.Count;
			m_msaa.quality = desc.SampleDesc.Quality;
			m_desc = Tr2BitmapDimensions(
				Tr2RenderContextEnum::TEX_TYPE_2D,
				Tr2RenderContextEnum::PixelFormat( desc.Format ),
				uint32_t( desc.Width ),
				uint32_t( desc.Height ),
				1,
				1
			);
			m_textures = backBuffers;
			m_rtv = rtvs;
			m_owner = &renderContext;
			m_cpuUsage = Tr2CpuUsage::READ;
			m_gpuUsage = Tr2GpuUsage::RENDER_TARGET;
			m_defaultState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
	}

	void Tr2TextureAL::SetSwapChainBufferIndexDx12( uint32_t index )
	{
		m_currentTextureIndex = index;
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
		auto length = strlen( name );
		for( auto& t : m_textures )
		{
			SetDebugName( t, name );
		}
		return S_OK;
	}

	bool Tr2TextureAL::operator==( const Tr2TextureAL& other ) const
	{
		return GetResourceDx12() == other.GetResourceDx12();
	}
}

#endif