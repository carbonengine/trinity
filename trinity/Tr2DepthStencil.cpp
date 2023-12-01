#include "StdAfx.h"
#include "Tr2DepthStencil.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencil::Tr2DepthStencil( IRoot* )
	:m_width( 0 ),
	m_height( 0 ),
	m_format( Tr2RenderContextEnum::DSFMT_AUTO ),
	m_flags( Tr2RenderContextEnum::EX_NONE )
{	
}

Tr2DepthStencil::~Tr2DepthStencil()
{
}

void Tr2DepthStencil::SetName( const char* name )
{
	m_name = name;
	m_depthStencil.SetName( m_name.c_str() ).GetResult();
}

std::string Tr2DepthStencil::GetName() const
{
	return m_name;
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void Tr2DepthStencil::py__init__(
		Be::OptionalWithDefaultValue<unsigned, 0> width,
		Be::OptionalWithDefaultValue<unsigned, 0> height,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::DepthStencilFormat, Tr2RenderContextEnum::DSFMT_UNKNOWN> format,
		Be::OptionalWithDefaultValue<unsigned, 1> msaaType,
		Be::OptionalWithDefaultValue<unsigned, 0> msaaQuality,
		Be::OptionalWithDefaultValue<Tr2RenderContextEnum::ExFlag, Tr2RenderContextEnum::EX_NONE> flags )
{
	if( width && height && format )
	{
		Create( width, height, format, msaaType, msaaQuality, flags );		
	}		
}

long Tr2DepthStencil::Create( unsigned width, unsigned height, DepthStencilFormat dsFormat, unsigned msaaType, unsigned msaaQuality, Tr2RenderContextEnum::ExFlag flags )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	auto gpuUsage = Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE;
	if( ( flags & Tr2RenderContextEnum::EX_CREATE_SHARED ) != 0 )
	{
		gpuUsage |= Tr2GpuUsage::SHARED;
	}

	auto hr = m_depthStencil.Create( Tr2BitmapDimensions( width, height, 1, Tr2RenderContextEnum::ConvertDepthStencilFormat( dsFormat ) ), Tr2MsaaDesc( msaaType, msaaQuality ), gpuUsage, renderContext ).GetResult();
	if( SUCCEEDED( hr ) )
	{
		m_width = width;
		m_height = height;
		m_format = dsFormat;
		m_msaa = Tr2MsaaDesc( msaaType, msaaQuality );
		m_flags = flags;
		m_depthStencil.SetName( m_name.c_str() );
	}
	else
	{
		Destroy();
	}
	return hr;
}

Tr2TextureAL* Tr2DepthStencil::GetTexture()
{
	if( m_depthStencil.IsValid() && Tr2GpuUsage::HasFlag( m_depthStencil.GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE ) )
	{
		return  &m_depthStencil;
	}
	return nullptr;
}

bool Tr2DepthStencil::IsReadable() const
{
	return Tr2GpuUsage::HasFlag( m_depthStencil.GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE );
}

bool Tr2DepthStencil::IsValid() const
{
	return m_depthStencil.IsValid();
}	

void Tr2DepthStencil::Destroy()
{
	m_depthStencil = Tr2TextureAL();

	m_width = 0;
	m_height = 0;
	m_format = DSFMT_AUTO;
	m_msaa = Tr2MsaaDesc();
	m_flags = EX_NONE;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the object contains a reference to given AL object. This method is exposed
//   to Python and is used for debugging.
// Arguments:
//   type - Tr2RenderContextEnum::ObjectType, type of AL object
//   object - pointer to an AL object (passed as a number)
// Return Value:
//   true If object contains a reference to the given AL object
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DepthStencil::HasALObject( int type, size_t object )
{
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns DirectX shared handle of contained AL depth stencil.
// Return Value:
//   Shared handle of contained AL depth stencil
// --------------------------------------------------------------------------------------
uintptr_t Tr2DepthStencil::GetSharedHandle() const
{
	return m_depthStencil.GetSharedHandle();
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetWidth() const
{
	return m_depthStencil.GetWidth();
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetHeight() const
{
	return m_depthStencil.GetHeight();
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetMsaaSamples() const
{
	return m_depthStencil.GetMsaaDesc().samples;
}

// --------------------------------------------------------------------------------------
uint32_t Tr2DepthStencil::GetMsaaQuality() const
{
	return m_depthStencil.GetMsaaDesc().quality;
}

uint32_t Tr2DepthStencil::GetMipCount() const 
{ 
	return 1; 
}

// --------------------------------------------------------------------------------------
Tr2RenderContextEnum::DepthStencilFormat Tr2DepthStencil::GetFormat() const
{
	return m_format;
}

// --------------------------------------------------------------------------------------
void Tr2DepthStencil::ReleaseResources( TriStorage s )
{
	if( m_depthStencil.IsValid() && ( s & m_depthStencil.GetMemoryClass() ) )
	{
		m_depthStencil = Tr2TextureAL();
	}
}

// --------------------------------------------------------------------------------------
bool Tr2DepthStencil::OnPrepareResources()
{
	if( !m_depthStencil.IsValid() && m_width != 0 )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		auto gpuUsage = Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE;
		if( ( m_flags & Tr2RenderContextEnum::EX_CREATE_SHARED ) != 0 )
		{
			gpuUsage |= Tr2GpuUsage::SHARED;
		}

		m_depthStencil.Create( Tr2BitmapDimensions( m_width, m_height, 1, Tr2RenderContextEnum::ConvertDepthStencilFormat( m_format ) ), m_msaa, gpuUsage, renderContext );
	}
	return true;
}
