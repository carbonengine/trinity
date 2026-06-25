// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "Tr2GpuStructuredBuffer.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GpuStructuredBuffer default constructor
// --------------------------------------------------------------------------------------
Tr2GpuStructuredBuffer::Tr2GpuStructuredBuffer( IRoot* ) :
	m_count( 0 ),
	m_stride( 0 ),
	m_creationFlags( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GpuStructuredBuffer destructor
// --------------------------------------------------------------------------------------
Tr2GpuStructuredBuffer::~Tr2GpuStructuredBuffer()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
ALResult Tr2GpuStructuredBuffer::py__init__( uint32_t count, uint32_t stride, CreationFlags creationFlags )
{
	if( count && stride )
	{
		return Create( count, stride, creationFlags );
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Construct AL buffer from read parameters.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GpuStructuredBuffer::Initialize()
{
	CreateBuffer();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface. Re-creates AL buffer when of the object parameters
//   changes.
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GpuStructuredBuffer::OnModified( Be::Var* value )
{
	CreateBuffer();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns new values to buffer parameters and construct AL buffer.
// Arguments:
//   count - Number of elements in the buffer
//   stride - Size of one element in bytes
//   creationFlags - Creation flags (see Tr2GpuStructuredBuffer::CreationFlag)
// Return Value:
//   true If AL buffer is successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
ALResult Tr2GpuStructuredBuffer::Create( uint32_t count, uint32_t stride, CreationFlags creationFlags )
{
	m_count = count;
	m_stride = stride;
	m_creationFlags = creationFlags;
	return CreateBuffer();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GpuBuffer interface. Returns AL buffer.
// Arguments:
//   index - Buffer index (unused)
// Return Value:
//   Pointer to AL buffer
// --------------------------------------------------------------------------------------
Tr2BufferAL* Tr2GpuStructuredBuffer::GetGpuBuffer( unsigned index )
{
	return m_buffer.IsValid() ? &m_buffer : nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if the object contains a valid AL buffer.
// Return Value:
//   true If the object contains a valid AL buffer
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2GpuStructuredBuffer::IsValid() const
{
	return m_buffer.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-creates AL buffer.
// Return Value:
//   true If AL buffer is successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
ALResult Tr2GpuStructuredBuffer::CreateBuffer()
{
	if( !m_count || !m_stride )
	{
		return E_INVALIDARG;
	}

	auto gpuUsage = Tr2GpuUsage::SHADER_RESOURCE;
	auto cpuUsage = Tr2CpuUsage::READ;
	if( m_creationFlags & GPU_WRITABLE )
	{
		gpuUsage = gpuUsage | Tr2GpuUsage::UNORDERED_ACCESS;
	}
	else if( m_creationFlags & CPU_WRITABLE )
	{
		cpuUsage = Tr2CpuUsage::WRITE_OFTEN;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	auto hr = m_buffer.Create(
		m_stride,
		m_count,
		gpuUsage,
		cpuUsage,
		nullptr,
		renderContext );
	if( !m_name.empty() && SUCCEEDED( hr ) )
	{
		m_buffer.SetName( m_name.c_str() );
	}
	return hr;
}

void Tr2GpuStructuredBuffer::SetName( const char* name )
{
	m_name = name;
	if( m_buffer.IsValid() )
	{
		m_buffer.SetName( name );
	}
}

void Tr2GpuStructuredBuffer::ReleaseResources( TriStorage )
{
}

bool Tr2GpuStructuredBuffer::OnPrepareResources()
{
	if( !m_buffer.IsValid() && m_count != 0 && m_stride != 0 )
	{
		return SUCCEEDED( CreateBuffer() );
	}
	return true;
}

uint32_t Tr2GpuStructuredBuffer::GetCount() const
{
	return m_buffer.GetDesc().count;
}