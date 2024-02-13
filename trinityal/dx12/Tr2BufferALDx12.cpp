////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2BufferALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


namespace TrinityALImpl
{
	Tr2BufferAL::Tr2BufferAL()
		:m_owner( nullptr ),
		m_defaultState( D3D12_RESOURCE_STATE_COMMON )
	{
		memset( &m_srvDesc, 0, sizeof( m_srvDesc ) );
		memset( &m_uavDesc, 0, sizeof( m_uavDesc ) );
		m_desc.count = 0;
	}

	Tr2BufferAL::~Tr2BufferAL()
	{
		Destroy();
	}

	ALResult Tr2BufferAL::Create(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( desc.count == 0 )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		bool isImmutable = !HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) && !HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS );
		if( isImmutable && !initialData )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( desc.cpuUsage, Tr2CpuUsage::READ ) && HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDARG;
		}

		auto stride = desc.stride;
		if( desc.format != Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
		{
			stride = GetBytesPerPixel( desc.format );
		}

		auto size = desc.count * stride;

		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if( !HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		D3D12_RESOURCE_STATES defaultState;
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::ACCELERATION_STRUCTURE ) )
		{
			defaultState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		}
		else if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) ||
			HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) ||
			HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE )
			)
		{
			defaultState = D3D12_RESOURCE_STATE_COMMON;

			if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) )
			{
				defaultState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			}
			if( HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) )
			{
				defaultState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
			}
			if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
			{
				defaultState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
		}
		else if( HasFlag( desc.gpuUsage, Tr2GpuUsage::DRAW_INDIRECT_ARGS ) )
		{
			defaultState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		}
		else if( HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			defaultState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		else
		{
			defaultState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		D3D12_SUBRESOURCE_DATA subresourceData = { initialData, size, size };

		Tr2ResourceHelper::Strategy strategy;
		if( !HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			strategy = Tr2ResourceHelper::STATIC;
		}
		else if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) || HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			strategy = Tr2ResourceHelper::SEMI_DYNAMIC;
		}
		else
		{
			strategy = Tr2ResourceHelper::DYNAMIC;
			defaultState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		FORWARD_HR( m_buffer.Create( strategy, size, resourceFlags, defaultState, RequiresImmediateBarriers( desc.gpuUsage ), initialData ? 1 : 0, initialData ? &subresourceData : nullptr, renderContext ) );

		m_desc = desc;
		m_owner = &renderContext;
		m_defaultState = defaultState;

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			m_srvDesc.Format = DXGI_FORMAT( desc.format );
			m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			if( HasFlag( desc.gpuUsage, Tr2GpuUsage::ACCELERATION_STRUCTURE ) )
			{
				m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
				m_srvDesc.RaytracingAccelerationStructure.Location = m_buffer.GetGpuView();
			}
			else
			{
				m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				m_srvDesc.Buffer.FirstElement = 0;
				m_srvDesc.Buffer.NumElements = desc.count;
				m_srvDesc.Buffer.StructureByteStride = m_srvDesc.Format == DXGI_FORMAT_UNKNOWN ? stride : 0;
				m_srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) )
				{
					m_srvDesc.Buffer.StructureByteStride = 4;
					m_srvDesc.Buffer.NumElements = size / 4;
				}
			}

			HRESULT hr;
			if (FAILED(hr = renderContext.CreateShaderResourceView(m_buffer.GetResource(), m_srvDesc, m_srv)))
			{
				Destroy();
				return hr;
			}
		}
		else
		{
			memset( &m_srvDesc, 0, sizeof( m_srvDesc ) );
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC clearUav;
			if( desc.format != Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
			{
				clearUav.Format = DXGI_FORMAT( desc.format );
				clearUav.Buffer.NumElements = desc.count;
			}
			else
			{
				clearUav.Format = DXGI_FORMAT_R32_UINT;
				clearUav.Buffer.NumElements = ( desc.count * desc.stride ) / 4;
			}
			clearUav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			clearUav.Buffer.FirstElement = 0;
			clearUav.Buffer.StructureByteStride = 0;
			clearUav.Buffer.CounterOffsetInBytes = 0;
			clearUav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			HRESULT hr;
			if (FAILED(hr = renderContext.CreateUnorderedAccessView(m_buffer.GetResource(), nullptr, clearUav, m_clearUav)))
			{
				Destroy();
				return hr;
			}


			m_uavDesc.Format = DXGI_FORMAT( desc.format );
			m_uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			m_uavDesc.Buffer.FirstElement = 0;
			m_uavDesc.Buffer.NumElements = desc.count;
			m_uavDesc.Buffer.StructureByteStride = m_uavDesc.Format == DXGI_FORMAT_UNKNOWN ? stride : 0;
			m_uavDesc.Buffer.CounterOffsetInBytes = 0;
			m_uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			if (FAILED(hr = renderContext.CreateUnorderedAccessView(m_buffer.GetResource(), nullptr, m_uavDesc, m_uav)))
			{
				Destroy();
				return hr;
			}
		}
		else
		{
			memset( &m_uavDesc, 0, sizeof( m_uavDesc ) );
		}

		return S_OK;
	}

	void Tr2BufferAL::Destroy()
	{
		if( m_owner )
		{
			m_buffer.Destroy( *m_owner );
		}
		if( m_lockedScratch )
		{
			RELEASE_LATER( m_owner, m_lockedScratch );
			m_lockedScratch = nullptr;
		}

		memset( &m_srvDesc, 0, sizeof( m_srvDesc ) );
		memset( &m_uavDesc, 0, sizeof( m_uavDesc ) );
		m_srv = nullptr;
		m_uav = nullptr;
		m_clearUav = nullptr;
		m_desc.count = 0;
		m_owner = nullptr;
		m_defaultState = D3D12_RESOURCE_STATE_COMMON;
	}

	bool Tr2BufferAL::IsValid() const
	{
		return m_owner != nullptr;
	}

	Tr2ALMemoryType Tr2BufferAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	const Tr2BufferDescriptionAL& Tr2BufferAL::GetDesc() const
	{
		return m_desc;
	}

	ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}
		return m_buffer.MapForWriting( data, HasFlag( m_desc.cpuUsage, Tr2CpuUsage::NON_SYNCRONIZED_WRITE ) ? Tr2LockType::NON_SYNCHRONIZED : Tr2LockType::SYNCHRONIZED, *m_owner );
	}

	void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& )
	{
		m_buffer.UnmapForWriting( *m_owner );
	}

	ALResult Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
	{
		auto stride = m_desc.stride;
		if( m_desc.format != Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
		{
			stride = GetBytesPerPixel( m_desc.format );
		}
		if( m_desc.count * stride < size )
		{
			return E_INVALIDARG;
		}

		if( !size )
		{
			size = m_desc.count * stride;
		}

		return m_buffer.UpdateBuffer( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::NON_SYNCRONIZED_WRITE ) ? Tr2LockType::NON_SYNCHRONIZED : Tr2LockType::SYNCHRONIZED, offset, size, data, renderContext );
	}

	ALResult Tr2BufferAL::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}
		if( m_lockedScratch )
		{
			return E_INVALIDCALL;
		}

		CComPtr<ID3D12Resource> scratch;
		auto stride = m_desc.stride;
		if( m_desc.format != Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
		{
			stride = GetBytesPerPixel( m_desc.format );
		}
		auto size = m_desc.count * stride;

		auto scratchDesc = BufferDesc( size );
		auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_READBACK );
		CR_RETURN_HR( m_owner->m_device->CreateCommittedResource(
			&scratchHeap,
			D3D12_HEAP_FLAG_NONE,
			&scratchDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &scratch ) ) );

		renderContext.ResourceBarrierDx12( Transition( m_buffer.GetResource(), m_defaultState, D3D12_RESOURCE_STATE_COPY_SOURCE ) );
		renderContext.FlushBarriersDx12( m_buffer.GetResource() );
		renderContext.m_commandList->CopyBufferRegion( scratch, 0, m_buffer.GetResource(), 0, size );
		renderContext.ResourceBarrierDx12( Transition( m_buffer.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, m_defaultState ) );
		if( RequiresImmediateBarriers( m_desc.gpuUsage ) )
		{
			renderContext.FlushBarriersDx12( m_buffer.GetResource() );
		}

		auto hr = renderContext.FlushAndSyncDx12();
		if( FAILED( hr ) )
		{
			RELEASE_LATER( m_owner, scratch );
			scratch = nullptr;
			return hr;
		}

		CR_RETURN_HR( scratch->Map( 0, nullptr, (void**)&data ) );
		m_lockedScratch = scratch;

		return S_OK;
	}

	void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& )
	{
		if( !m_lockedScratch )
		{
			return;
		}
		D3D12_RANGE range = { 0, 0 };
		m_lockedScratch->Unmap( 0, &range );
		RELEASE_LATER( m_owner, m_lockedScratch );
		m_lockedScratch = nullptr;
	}

	ID3D12Resource* Tr2BufferAL::GetGpuResource()
	{
		return m_buffer.GetResource();
	}

	D3D12_GPU_VIRTUAL_ADDRESS Tr2BufferAL::GetGpuView()
	{
		return m_buffer.GetGpuView();
	}

	uint32_t Tr2BufferAL::GetSrvIndexInHeap() const
	{
		if( m_srv )
		{
			return m_srv->GetIndexInHeap();
		}
		else
		{
			return 0xffffffff;
		}
	}
	
	uint32_t Tr2BufferAL::GetUavIndexInHeap() const
	{
		if( m_uav )
		{
			return m_uav->GetIndexInHeap();
		}
		else
		{
			return 0xffffffff;
		}
	}

	void Tr2BufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2BufferAL";
		description["size"] = std::to_string( long long( GetDesc().count * GetDesc().stride ) );
		description["cpuUsage"] = std::to_string( int( GetDesc().cpuUsage ) );
		description["gpuUsage"] = std::to_string( int( GetDesc().gpuUsage ) );
		description["format"] = std::to_string( int( GetDesc().format ) );
		description["stride"] = std::to_string( GetDesc().stride );
		description["count"] = std::to_string( GetDesc().count );
		description["name"] = m_buffer.GetName();
	}

	ALResult Tr2BufferAL::SetName( const char* name )
	{
		m_buffer.SetName( name );
		return S_OK;
	}

	D3D12_RESOURCE_STATES Tr2BufferAL::GetDefaultState() const
	{
		return m_defaultState;
	}
}

#endif