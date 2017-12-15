#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2GpuBufferALDx11.h"
#include "ALLog.h"
#include "Tr2PrimaryRenderContextDx11.h"
#include "include/Tr2VertexBufferAL.h"

using namespace Tr2RenderContextEnum;

Tr2GpuBufferAL::Tr2GpuBufferAL()
	: m_numElements( 0 )
	, m_elementSize( 0 )
	, m_format( PIXEL_FORMAT_UNKNOWN )
	, m_gpuBufferUsage( 0 )
{
}

ALResult Tr2GpuBufferAL::Create(	
	uint32_t numberOfElements, 
	Tr2RenderContextEnum::PixelFormat format, 
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2RenderContextEnum::ExFlag flags,
	const void* initialData,
	Tr2PrimaryRenderContextAL & renderContext )
{
	Destroy();

	m_numElements = numberOfElements;
	m_elementSize = 0;
	m_format = format;
	m_lengthInBytes = GetTotalSizeInBytes();
	m_gpuBufferUsage = 0;

	return CreateImpl( usage, 0, initialData, flags, renderContext );
}
	
ALResult Tr2GpuBufferAL::Create(	
	uint32_t numberOfElements, 
	uint32_t elementSize, 
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
	Tr2RenderContextEnum::ExFlag flags,
	const void* initialData,
	Tr2PrimaryRenderContextAL & renderContext )
{
	Destroy();

	m_numElements = numberOfElements;
	m_elementSize = elementSize;
	m_format = PIXEL_FORMAT_UNKNOWN;
	m_lengthInBytes = GetTotalSizeInBytes();
	m_gpuBufferUsage = gpuBufferUsage;

	return CreateImpl( usage, gpuBufferUsage, initialData, flags, renderContext );
}
	
ALResult Tr2GpuBufferAL::CreateImpl( 
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
	const void* initialData, 
	uint32_t flags,
	Tr2PrimaryRenderContextAL &renderContext )
{
	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable UAV buffer without providing data" );
		return E_INVALIDARG;
	}

	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2GpuBufferAL Create function" );
		return E_INVALIDARG;
	}

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	const bool writable = ( usage & USAGE_CPU_WRITE ) != 0;

	if( writable && gpuBufferUsage )
	{
		return E_INVALIDARG;
	}
	
	D3D11_BUFFER_DESC desc;
	memset( &desc, 0, sizeof( desc ) );

	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	if( !writable && ( usage & USAGE_IMMUTABLE ) == 0 )
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	desc.MiscFlags = m_elementSize ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0;
	if( flags & EX_DRAW_INDIRECT )
	{
		desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	}

	desc.ByteWidth = GetTotalSizeInBytes();
	desc.StructureByteStride = BytesPerElement();

	if( usage & USAGE_IMMUTABLE )
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.CPUAccessFlags = 0;
	}
	else if( usage & USAGE_LOCK_FREQUENTLY )
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = initialData;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateBuffer( &desc, initialData ? &data : nullptr, &m_buffer ) );

	// Structured Buffer binding
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	memset( &descSRV, 0, sizeof( descSRV ) );
	descSRV.Format = static_cast<DXGI_FORMAT>( m_format );
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.ElementWidth = m_elementSize;
	descSRV.Buffer.NumElements = m_numElements;
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateShaderResourceView( m_buffer, &descSRV, &m_srv ) );
	
	if( !writable )
	{
		// Unstructured Buffer binding
		D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;
		descUAV.Format = static_cast<DXGI_FORMAT>( m_format );
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.NumElements  = m_numElements;
		descUAV.Buffer.Flags = 0;
		if( gpuBufferUsage & GPU_BUFFER_APPEND )
		{
			descUAV.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
		}
		if( gpuBufferUsage & GPU_BUFFER_COUNTER )
		{
			descUAV.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER;
		}
		CR_RETURN_HR( renderContext.m_d3dDevice11->CreateUnorderedAccessView( m_buffer, &descUAV, &m_uav ) );
	}

	m_usage = usage;

	m_memory.Set( Tr2MemoryCounterAL::BUFFER, GetTotalSizeInBytes() );
	ChangeObjectId();

	return S_OK;
}

ALResult Tr2GpuBufferAL::CreateVbView(	
	Tr2VertexBufferAL& vb,
	bool gpuWritable,
	Tr2PrimaryRenderContextAL & renderContext )
{
	Destroy();
	if( !vb.IsValid() )
	{
		return E_INVALIDARG;
	}

	m_format			= PIXEL_FORMAT_R32_UINT;
	m_buffer			= vb.m_buffer;
	m_usage				= 0;
	m_lengthInBytes		= vb.GetTotalSizeInBytes();
	m_elementSize		= 4;
	m_numElements		= m_lengthInBytes / 4;
	m_gpuBufferUsage = 0;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	memset( &descSRV, 0, sizeof( descSRV ) );
	descSRV.Format = static_cast<DXGI_FORMAT>( m_format );
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	descSRV.Buffer.ElementWidth = m_elementSize;
	descSRV.Buffer.NumElements = m_numElements;
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateShaderResourceView( m_buffer, &descSRV, &m_srv ) );
	
	if( gpuWritable )
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;
		descUAV.Format = static_cast<DXGI_FORMAT>( m_format );
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Buffer.NumElements  = m_numElements;
		descUAV.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		CR_RETURN_HR( renderContext.m_d3dDevice11->CreateUnorderedAccessView( m_buffer, &descUAV, &m_uav ) );
	}
	ChangeObjectId();
	return S_OK;
}

void Tr2GpuBufferAL::Destroy()
{
	Tr2BufferImplAL::Destroy();
	m_srv = nullptr;
	m_uav = nullptr;
	m_numElements = 0;
	m_elementSize = 0;
	m_lengthInBytes = 0;
	m_format = PIXEL_FORMAT_UNKNOWN;
	m_gpuBufferUsage = 0;
}

Tr2GpuBufferAL& Tr2GpuBufferAL::operator=( Tr2GpuBufferAL&& other )
{
	m_numElements	= other.m_numElements;
	m_format		= other.m_format;
	m_elementSize = other.m_elementSize;
	Tr2BufferImplAL::operator=( std::move( other ) );
	m_uav = std::move( other.m_uav );
	m_srv = std::move( other.m_srv );
	m_gpuBufferUsage = m_gpuBufferUsage;
	return *this;
}

bool Tr2GpuBufferAL::IsValid() const
{ 
	if( m_elementSize == 0 )	// unstructured
	{
		return	m_buffer		!= nullptr		&& 
				m_numElements	!= 0			&& 
				m_format		!= Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN &&
				( m_uav	!= nullptr || m_srv	!= nullptr  );
	}

	return	m_buffer		!= nullptr	&& 			
			m_numElements	!= 0		&&
			( m_uav	!= nullptr || m_srv	!= nullptr  );
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies part of the buffer into destination buffer (using GPU).
// Arguments:
//   offset - Byte offset into the buffer to the data to be copied
//   length - Byte length of the data to be copied
//   dest - Destination buffer
//   destOffset - Byte offset into destination buffer
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2GpuBufferAL::CopySubBuffer( 
	uint32_t offset, 
	uint32_t length, 
	Tr2GpuBufferAL& dest, 
	uint32_t destOffset, 
	Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !dest.IsValid() || !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	D3D11_BOX srcBox = { offset, 0, 0, offset + length, 1, 1, };
	renderContext.m_context->CopySubresourceRegion( dest.m_buffer, 
													0, 
													destOffset, 
													0, 
													0, 
													m_buffer, 
													0, 
													&srcBox );
	return S_OK;
}

uint32_t Tr2GpuBufferAL::BytesPerElement() const
{
	return m_elementSize ? m_elementSize : GetBytesPerPixel( m_format );
}

uint32_t Tr2GpuBufferAL::GetNumElements() const
{
	return m_numElements;
}

uint32_t Tr2GpuBufferAL::GetTotalSizeInBytes() const
{
	return GetNumElements() * BytesPerElement();
}

Tr2RenderContextEnum::PixelFormat Tr2GpuBufferAL::GetFormat() const
{
	return m_format;
}

Tr2RenderContextEnum::GpuBufferUsage Tr2GpuBufferAL::GetGpuBufferUsage() const
{
	return m_gpuBufferUsage;
}

bool Tr2GpuBufferAL::operator==( const Tr2GpuBufferAL& other ) const
{
	return m_buffer == other.m_buffer;
}

Tr2ALMemoryType Tr2GpuBufferAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}


#endif