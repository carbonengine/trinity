// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2BufferAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2BufferAL )


using namespace Tr2RenderContextEnum;

Tr2BufferDescriptionAL::Tr2BufferDescriptionAL() :
	format( PIXEL_FORMAT_UNKNOWN ),
	stride( 0 ),
	count( 0 ),
	gpuUsage( Tr2GpuUsage::NONE ),
	cpuUsage( Tr2CpuUsage::NONE )
{
}

Tr2BufferDescriptionAL::Tr2BufferDescriptionAL(
	Tr2RenderContextEnum::PixelFormat format_,
	uint32_t count_,
	Tr2GpuUsage::Type gpuUsage_,
	Tr2CpuUsage::Type cpuUsage_ ) :
	format( format_ ),
	stride( GetBytesPerPixel( format_ ) ),
	count( count_ ),
	gpuUsage( gpuUsage_ ),
	cpuUsage( cpuUsage_ )
{
}

Tr2BufferDescriptionAL::Tr2BufferDescriptionAL(
	uint32_t stride_,
	uint32_t count_,
	Tr2GpuUsage::Type gpuUsage_,
	Tr2CpuUsage::Type cpuUsage_ ) :
	format( PIXEL_FORMAT_UNKNOWN ),
	stride( stride_ ),
	count( count_ ),
	gpuUsage( gpuUsage_ ),
	cpuUsage( cpuUsage_ )
{
}


namespace
{
std::shared_ptr<TrinityALImpl::Tr2BufferAL> nullBuffer = std::make_shared<TrinityALImpl::Tr2BufferAL>();
}

Tr2BufferAL::Tr2BufferAL() :
	m_buffer( nullBuffer )
{
	if( m_buffer == nullptr )
	{
		m_buffer = std::make_shared<TrinityALImpl::Tr2BufferAL>();
	}
}

ALResult Tr2BufferAL::Create(
	const Tr2BufferDescriptionAL& desc,
	const void* initialData,
	Tr2PrimaryRenderContextAL& renderContext )
{
	m_buffer = std::make_shared<TrinityALImpl::Tr2BufferAL>();
	auto result = m_buffer->Create( desc, initialData, renderContext );
	if( FAILED( result ) )
	{
		m_buffer = nullBuffer;
	}
	return result;
}

ALResult Tr2BufferAL::Create(
	Tr2RenderContextEnum::PixelFormat format,
	uint32_t count,
	Tr2GpuUsage::Type gpuUsage,
	Tr2CpuUsage::Type cpuUsage,
	const void* initialData,
	Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( Tr2BufferDescriptionAL( format, count, gpuUsage, cpuUsage ), initialData, renderContext );
}

ALResult Tr2BufferAL::Create(
	uint32_t stride,
	uint32_t count,
	Tr2GpuUsage::Type gpuUsage,
	Tr2CpuUsage::Type cpuUsage,
	const void* initialData,
	Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( Tr2BufferDescriptionAL( stride, count, gpuUsage, cpuUsage ), initialData, renderContext );
}

bool Tr2BufferAL::IsValid() const
{
	return m_buffer->IsValid();
}

Tr2ALMemoryType Tr2BufferAL::GetMemoryClass()
{
	return m_buffer->GetMemoryClass();
}

const Tr2BufferDescriptionAL& Tr2BufferAL::GetDesc() const
{
	return m_buffer->GetDesc();
}

uint32_t Tr2BufferAL::GetSize() const
{
	return GetDesc().count * GetDesc().stride;
}

bool Tr2BufferAL::operator==( const Tr2BufferAL& other ) const
{
	return m_buffer == other.m_buffer;
}

ALResult Tr2BufferAL::MapForReading( const void*& data, Tr2RenderContextAL& renderContext ) const
{
	return m_buffer->MapForReading( data, renderContext );
}

ALResult Tr2BufferAL::MapForReading( const void*& data, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext ) const
{
	return m_buffer->MapForReading( data, offset, size, renderContext );
}

void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& renderContext ) const
{
	m_buffer->UnmapForReading( renderContext );
}

ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2RenderContextAL& renderContext )
{
	return m_buffer->MapForWriting( data, renderContext );
}

void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
{
	m_buffer->UnmapForWriting( renderContext );
}

ALResult Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL& renderContext )
{
	return m_buffer->UpdateBuffer( offset, size, data, renderContext );
}

uint32_t Tr2BufferAL::GetSrvIndexInHeap() const
{
	return m_buffer->GetSrvIndexInHeap();
}

uint32_t Tr2BufferAL::GetUavIndexInHeap() const
{
	return m_buffer->GetUavIndexInHeap();
}

ALResult Tr2BufferAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_buffer->SetName( name );
}

TrinityALImpl::Tr2BufferAL* Tr2BufferAL::TrinityALImpl_GetObject() const
{
	return m_buffer.get();
}