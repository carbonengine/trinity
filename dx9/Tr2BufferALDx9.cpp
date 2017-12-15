#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX9

#include "Tr2BufferALDx9.h"
#include "Tr2RenderContextDx9.h"

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

		if( ( desc.gpuUsage & ( ~Tr2GpuUsage::VERTEX_BUFFER & ~Tr2GpuUsage::INDEX_BUFFER ) ) != 0 )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) && HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) )
		{
			return E_INVALIDARG;
		}

		if( desc.count == 0 )
		{
			return E_INVALIDARG;
		}

		if( !HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		unsigned dxUsage = 0;
		auto pool = D3DPOOL_DEFAULT;
		if( HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			dxUsage = D3DUSAGE_DYNAMIC;
		}
		else if( !renderContext.m_usingEXDevice )
		{
			pool = D3DPOOL_MANAGED;
		}
		if( !HasFlag( desc.cpuUsage, Tr2CpuUsage::READ ) )
		{
			dxUsage |= D3DUSAGE_WRITEONLY;
		}

		CComPtr<IDirect3DIndexBuffer9> indexBuffer;
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) )
		{
			auto format = desc.stride == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateIndexBuffer( desc.stride * desc.count, dxUsage, format, pool, &indexBuffer, 0 ) );
			if( initialData )
			{
				void* data = nullptr;
				CR_RETURN_HR( indexBuffer->Lock( 0, 0, &data, 0 ) );
				if( !data )
				{
					return E_FAIL;
				}
				memcpy( data, initialData, desc.stride * desc.count );
				CR_RETURN_HR( indexBuffer->Unlock() );
			}
		}

		CComPtr<IDirect3DVertexBuffer9> vertexBuffer;
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) )
		{
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateVertexBuffer( desc.stride * desc.count, dxUsage, 0, pool, &vertexBuffer, 0 ) );
			if( initialData )
			{
				void* data = nullptr;
				CR_RETURN_HR( vertexBuffer->Lock( 0, 0, &data, 0 ) );
				if( !data )
				{
					return E_FAIL;
				}
				memcpy( data, initialData, desc.stride * desc.count );
				CR_RETURN_HR( vertexBuffer->Unlock() );
			}
		}

		m_indexBuffer = indexBuffer;
		m_vertexBuffer = vertexBuffer;
		m_pool = pool;
		m_desc = desc;
		m_memory.Set( Tr2MemoryCounterAL::BUFFER, desc.stride * desc.count );
		return S_OK;
	}

	void Tr2BufferAL::Destroy()
	{
		m_vertexBuffer = nullptr;
		m_indexBuffer = nullptr;
		m_memory.Reset();
		m_desc.count = 0;
	}

	bool Tr2BufferAL::IsValid() const
	{
		return m_vertexBuffer || m_indexBuffer;
	}

	Tr2ALMemoryType Tr2BufferAL::GetMemoryClass()
	{
		return m_pool == D3DPOOL_DEFAULT ? AL_MEMORY_VIDEO : AL_MEMORY_MANAGED;
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

		return Lock( const_cast<void**>( &data ), D3DLOCK_READONLY );
	}

	void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return;
		}
		Unlock();
	}

	ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			data = nullptr;
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}

		uint32_t flags = 0;
		if( lockType == Tr2LockType::NON_SYNCHRONIZED )
		{
			flags = D3DLOCK_NOOVERWRITE;
		}
		else if( HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			flags = D3DLOCK_DISCARD;
		}

		return Lock( &data, flags );
	}

	void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return;
		}
		Unlock();
	}

	ALResult Tr2BufferAL::Lock( void** data, uint32_t flags )
	{
		*data = nullptr;
		if( m_vertexBuffer )
		{
			CR_RETURN_HR( m_vertexBuffer->Lock( 0, 0, data, flags ) );
		}
		else
		{
			CR_RETURN_HR( m_indexBuffer->Lock( 0, 0, data, flags ) );
		}
		if( *data == nullptr )
		{
			return E_FAIL;
		}
		return S_OK;
	}

	void Tr2BufferAL::Unlock()
	{
		if( m_vertexBuffer )
		{
			m_vertexBuffer->Unlock();
		}
		else
		{
			m_indexBuffer->Unlock();
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
		if( m_vertexBuffer )
		{
			void* lockedData = nullptr;
			CR_RETURN_HR( m_vertexBuffer->Lock( offset, size, &lockedData, 0 ) );
			if( !lockedData )
			{
				m_vertexBuffer->Unlock();
				return E_FAIL;
			}
			memcpy( lockedData, data, size );
			return m_vertexBuffer->Unlock();
		}
		else if( m_indexBuffer )
		{
			void* lockedData = nullptr;
			CR_RETURN_HR( m_indexBuffer->Lock( offset, size, &lockedData, 0 ) );
			if( !lockedData )
			{
				m_indexBuffer->Unlock();
				return E_FAIL;
			}
			memcpy( lockedData, data, size );
			return m_indexBuffer->Unlock();
		}
		return E_FAIL;
	}
}

#endif