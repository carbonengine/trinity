#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2BufferALStub.h"
#include "Tr2RenderContextStub.h"

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

		m_desc = desc;
		m_buffer.resize( "Tr2BufferAL", m_desc.count * m_desc.stride );
		return S_OK;
	}

	void Tr2BufferAL::Destroy()
	{
		m_desc.count = 0;
		m_buffer.clear();
	}

	bool Tr2BufferAL::IsValid() const
	{
		return m_buffer.size() > 0;
	}

	Tr2ALMemoryType Tr2BufferAL::GetMemoryClass() const
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
		data = m_buffer.get();
		return S_OK;
	}

	void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& )
	{
	}

	ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2RenderContextAL& renderContext )
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
		data = m_buffer.get();
		return S_OK;
	}

	void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& )
	{
	}

	ALResult Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void*, Tr2RenderContextAL & renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( offset + size > m_desc.count * m_desc.stride )
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
		return S_OK;
	}

	uint32_t Tr2BufferAL::GetSrvIndexInHeap() const
	{
		return 0xffffffff;
	}

	uint32_t Tr2BufferAL::GetUavIndexInHeap() const
	{
		return 0xffffffff;
	}

	void Tr2BufferAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
	{
	}

	ALResult Tr2BufferAL::SetName( const char* )
	{
		return S_OK;
	}

}

#endif