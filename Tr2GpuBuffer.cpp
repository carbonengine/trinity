////////////////////////////////////////////////////////////
//
//    Created:   February 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "Tr2GpuBuffer.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GpuBuffer default constructor
// --------------------------------------------------------------------------------------
Tr2GpuBuffer::Tr2GpuBuffer( IRoot* )
	:m_count( 0 ),
	m_format( PIXEL_FORMAT_UNKNOWN ),
	m_creationFlags( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GpuBuffer destructor
// --------------------------------------------------------------------------------------
Tr2GpuBuffer::~Tr2GpuBuffer()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Construct AL buffer from read parameters.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GpuBuffer::Initialize()
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
bool Tr2GpuBuffer::OnModified( Be::Var* value )
{
	CreateBuffer();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Script-exposed initializer function. Assigns new values to buffer parameters and 
//   constructs AL buffer. Can be called with either 0, 2, 3 or 4 arguments.
// Arguments:
//   count - Number of elements in the buffer
//   format - Type of buffer elements
//   cpuWritable - Can the buffer be locked with write-only access
//   gpuWritable - Can the buffer be written into by GPU
//   drawIndirect - Is the buffer used for indirect draw calls
// Return Value:
//   true If AL buffer is successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
ALResult Tr2GpuBuffer::__init__( 
	Be::Optional<uint32_t> count, 
	Be::Optional<Tr2RenderContextEnum::PixelFormat> format, 
	CreationFlags creationFlags )
{
	if( count.IsAssigned() && !format.IsAssigned() )
	{
		return E_INVALIDARG;
	}
	if( format.IsAssigned() )
	{
		m_count = count;
		m_format = format;
		m_creationFlags = creationFlags;
		return CreateBuffer();
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns new values to buffer parameters and construct AL buffer.
// Arguments:
//   count - Number of elements in the buffer
//   format - Type of buffer elements
//   cpuWritable - Can the buffer be locked with write-only access
//   gpuWritable - Can the buffer be written into by GPU
//   drawIndirect - Is the buffer used for indirect draw calls
// Return Value:
//   true If AL buffer is successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
ALResult Tr2GpuBuffer::Create( uint32_t count, 
						   Tr2RenderContextEnum::PixelFormat format, 
						   CreationFlags creationFlags )
{
	m_count = count;
	m_format = format;
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
Tr2BufferAL* Tr2GpuBuffer::GetGpuBuffer( unsigned index )
{
	return &m_buffer;
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if the object contains a valid AL buffer.
// Return Value:
//   true If the object contains a valid AL buffer
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2GpuBuffer::IsValid() const
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
ALResult Tr2GpuBuffer::CreateBuffer()
{
	m_buffer = Tr2BufferAL();
	if( !m_count || m_format == PIXEL_FORMAT_UNKNOWN )
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
		cpuUsage = cpuUsage | Tr2CpuUsage::WRITE;
	}
	if( m_creationFlags & DRAW_INDIRECT )
	{
		gpuUsage = gpuUsage | Tr2GpuUsage::DRAW_INDIRECT_ARGS;
	}
	
	USE_MAIN_THREAD_RENDER_CONTEXT();
	auto hr = m_buffer.Create( 
		static_cast<PixelFormat>( m_format ), 
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

void Tr2GpuBuffer::ReleaseResources( TriStorage )
{
}

bool Tr2GpuBuffer::OnPrepareResources()
{
	if( !m_buffer.IsValid() && m_format != PIXEL_FORMAT_UNKNOWN )
	{
		return SUCCEEDED( CreateBuffer() );
	}
	return true;
}

uint32_t Tr2GpuBuffer::GetCount() const
{
	return m_buffer.GetDesc().count;
}

void Tr2GpuBuffer::SetName( const char* name )
{
	m_name = name;
	if( m_buffer.IsValid() )
	{
		m_buffer.SetName( name );
	}
}