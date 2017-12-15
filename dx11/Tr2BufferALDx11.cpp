#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2BufferALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"
#include "../ALLog.h"

using namespace Tr2RenderContextEnum;

namespace
{
	template <typename T>
	bool HasFlag( T value, T flag )
	{
		return ( value & flag ) == flag;
	}

}

namespace TrinityALImpl
{
	ALResult Tr2BufferAL::Create(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		auto stride = desc.stride;
		if( desc.format != PIXEL_FORMAT_UNKNOWN )
		{
			stride = GetBytesPerPixel( desc.format );
		}

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

		D3D11_BUFFER_DESC bd;
		memset( &bd, 0, sizeof( bd ) );

		if( isImmutable )
		{
			bd.Usage = D3D11_USAGE_IMMUTABLE;
		}
		else if( HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			bd.Usage = D3D11_USAGE_DEFAULT;
		}

		bd.ByteWidth = stride * desc.count;
		bd.StructureByteStride = stride;


		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) )
		{
			bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) )
		{
			bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			if( desc.format == PIXEL_FORMAT_UNKNOWN )
			{
				bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			}
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			bd.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::DRAW_INDIRECT_ARGS ) )
		{
			bd.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		}

		D3D11_SUBRESOURCE_DATA initialData11;
		if( initialData )
		{
			initialData11.pSysMem = initialData;
			initialData11.SysMemPitch = 0;
			initialData11.SysMemSlicePitch = 0;
		}

		auto hr = renderContext.m_d3dDevice11->CreateBuffer( &bd, initialData ? &initialData11 : nullptr, &m_buffer );
		if( hr == E_OUTOFMEMORY )
		{
			int retries = 10;
			while( hr == E_OUTOFMEMORY && retries )
			{
				CCP_AL_LOGWARN( "CreateBuffer failed with E_OUTOFMEMORY - retrying after Flush" );
				renderContext.m_context->Flush();
				hr = renderContext.m_d3dDevice11->CreateBuffer( &bd, initialData ? &initialData11 : nullptr, &m_buffer );
				--retries;
			}
		}
		if( FAILED( hr ) )
		{
			Destroy();
			return hr;
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
			memset( &descSRV, 0, sizeof( descSRV ) );
			descSRV.Format = static_cast<DXGI_FORMAT>( desc.format );
			descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			descSRV.Buffer.ElementWidth = stride;
			descSRV.Buffer.NumElements = desc.count;
			if( FAILED( renderContext.m_d3dDevice11->CreateShaderResourceView( m_buffer, &descSRV, &m_srv ) ) )
			{
				Destroy();
				return hr;
			}
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;
			descUAV.Format = static_cast<DXGI_FORMAT>( desc.format );
			descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			descUAV.Buffer.FirstElement = 0;
			descUAV.Buffer.NumElements = desc.count;
			descUAV.Buffer.Flags = 0;
			if( HasFlag( desc.gpuUsage, Tr2GpuUsage::APPEND_CONSUME ) )
			{
				descUAV.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
			}
			if( HasFlag( desc.gpuUsage, Tr2GpuUsage::BUFFER_COUNTER ) )
			{
				descUAV.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER;
			}
			if( FAILED( renderContext.m_d3dDevice11->CreateUnorderedAccessView( m_buffer, &descUAV, &m_uav ) ) )
			{
				Destroy();
				return hr;
			}
		}

		m_memory.Set( Tr2MemoryCounterAL::BUFFER, stride * desc.count );
		m_desc = desc;
		m_desc.stride = stride;

		return hr;
	}

	void Tr2BufferAL::Destroy()
	{
		m_memory.Reset();
		m_buffer = nullptr;
		m_staging = nullptr;
		m_srv = nullptr;
		m_uav = nullptr;
		m_desc.count = 0;
	}

	bool Tr2BufferAL::IsValid() const
	{
		return m_buffer != nullptr;
	}

	Tr2ALMemoryType Tr2BufferAL::GetMemoryClass()
	{
		return AL_MEMORY_MANAGED;
	}

	const Tr2BufferDescriptionAL& Tr2BufferAL::GetDesc() const
	{
		return m_desc;
	}

	ALResult Tr2BufferAL::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			data = nullptr;
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}
		if( !m_staging )
		{
			D3D11_BUFFER_DESC bd;
			memset( &bd, 0, sizeof( bd ) );

			bd.Usage = D3D11_USAGE_STAGING;
			bd.ByteWidth = m_desc.stride * m_desc.count;
			bd.BindFlags = 0;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

			CR_RETURN_HR(
				renderContext.m_secondaryDevice11->CreateBuffer(
					&bd,
					nullptr,
					&m_staging ) );

			if( !m_staging )
			{
				return E_FAIL;
			}
		}

		renderContext.m_context->CopyResource( m_staging, m_buffer );

		D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
		auto hr = renderContext.m_context->Map( m_staging, 0, D3D11_MAP_READ, 0, &ms );
		if( !ms.pData )
		{
			return E_FAIL;
		}
		data = ms.pData;
		return hr;
	}

	void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return;
		}
		if( !m_staging )
		{
			return;
		}
		renderContext.m_context->Unmap( m_staging, 0 );
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::READ_OFTEN ) )
		{
			m_staging = nullptr;
		}
	}

	ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext )
	{
		data = nullptr;
		if( !renderContext.IsValid() || !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}

		if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
			auto noSyncronization = lockType == Tr2LockType::NON_SYNCHRONIZED;
			auto mapType = noSyncronization ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
			auto hr = renderContext.m_context->Map( m_buffer, 0, mapType, 0, &ms );
			if( FAILED( hr ) )
			{
				return hr;
			}
			if( !ms.pData )
			{
				return E_FAIL;
			}
#if TRINITY_AL_GUARD_LOCKS
			if( noSyncronization )
			{
				data = ms.pData;
			}
			else
			{
				m_lockGuard.Lock( m_desc.count * m_desc.stride, ms.pData );
				data = m_lockGuard.GetMemory();
			}
#else
			data = ms.pData;
#endif
			return hr;
		}
		else
		{
			D3D11_BUFFER_DESC bd;
			memset( &bd, 0, sizeof( bd ) );

			bd.Usage = D3D11_USAGE_STAGING;
			bd.ByteWidth = m_desc.stride * m_desc.count;
			bd.BindFlags = 0;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

			CR_RETURN_HR(
				renderContext.m_secondaryDevice11->CreateBuffer(
					&bd,
					nullptr,
					&m_staging ) );

			if( !m_staging )
			{
				return E_FAIL;
			}

			D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
			auto hr = renderContext.m_context->Map( m_staging, 0, D3D11_MAP_WRITE, 0, &ms );
			if( !ms.pData )
			{
				return E_FAIL;
			}
#if TRINITY_AL_GUARD_LOCKS
			m_lockGuard.Lock( m_desc.count * m_desc.stride, ms.pData );
			data = m_lockGuard.GetMemory();
#else
			data = ms.pData;
#endif
			return hr;
		}
	}

	void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return;
		}
		if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
#if TRINITY_AL_GUARD_LOCKS
			if( m_lockGuard.GetMemory() )
			{
				m_lockGuard.Unlock();
			}
#endif
			renderContext.m_context->Unmap( m_buffer, 0 );
		}
		else
		{
#if TRINITY_AL_GUARD_LOCKS
			m_lockGuard.Unlock();
#endif
			renderContext.m_context->Unmap( m_staging, 0 );
			renderContext.m_context->CopyResource( m_buffer, m_staging );
		}
	}

	ALResult Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( offset + size > m_desc.stride * m_desc.count )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) || HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDCALL;
		}
		if( size == 0 )
		{
			return S_OK;
		}
		D3D11_BOX box = { 0, 0, 0, size, 1, 1 };
		renderContext.m_context->UpdateSubresource( m_buffer, 0, &box, data, 0, 0 );
		return S_OK;
	}

}
#endif