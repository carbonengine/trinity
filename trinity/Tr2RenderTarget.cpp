#include "StdAfx.h"
#include "Tr2RenderTarget.h"

using namespace Tr2RenderContextEnum;


namespace
{
	void GetUsage( uint32_t msaaType, Tr2RenderContextEnum::ExFlag flags, Tr2GpuUsage::Type& gpuUsage, Tr2CpuUsage::Type& cpuUsage )
	{
		gpuUsage = Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE;
		cpuUsage = Tr2CpuUsage::READ;
		if( msaaType > 1 )
		{
			cpuUsage = Tr2CpuUsage::NONE;
		}
		else if( ( flags & Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS ) != 0 )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::UNORDERED_ACCESS;
		}
		if( ( flags & Tr2RenderContextEnum::EX_CREATE_SHARED ) != 0 )
		{
			gpuUsage = gpuUsage | Tr2GpuUsage::SHARED;
		}
	}
}

Tr2RenderTarget::Tr2RenderTarget( IRoot* )
	:m_width( 0 ),
	m_height( 0 ),
	m_mipCount( 0 ),
	m_format( PIXEL_FORMAT_UNKNOWN ),
	m_flags( EX_NONE ),
	m_type( TEX_TYPE_INVALID )
{	
}

Tr2RenderTarget::~Tr2RenderTarget()
{
}

void Tr2RenderTarget::SetName( const char* name )
{
	m_name = name;
	m_renderTarget.SetName( m_name.c_str() ).GetResult();
}

std::string Tr2RenderTarget::GetName() const
{
	return m_name;
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void Tr2RenderTarget::py__init__( 
	unsigned width, 
	unsigned height, 
	unsigned mipCount, 
	Tr2RenderContextEnum::PixelFormat format,
	unsigned msaaType, 
	unsigned msaaQuality,
	Tr2RenderContextEnum::ExFlag flags,
	Tr2RenderContextEnum::TextureType type )
{
	if( width && height && format )
	{
		CCP_ASSERT( msaaType <= 1 || mipCount <= 1 );	// can't have msaa and mips at the same time.
		Create( width, height, mipCount, format, msaaType, msaaQuality, flags, type );
	}		
}

int32_t Tr2RenderTarget::Create(	
	unsigned width, 
	unsigned height, 
	unsigned mipLevelCount, 
	Tr2RenderContextEnum::PixelFormat format,
	unsigned msaaType,
	unsigned msaaQuality,
	Tr2RenderContextEnum::ExFlag flags,
	Tr2RenderContextEnum::TextureType type )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( IsAttached() )
	{
		return E_INVALIDARG;
	}

	auto gpuUsage = Tr2GpuUsage::NONE;
	auto cpuUsage = Tr2CpuUsage::NONE;
	GetUsage( uint32_t( msaaType ), flags, gpuUsage, cpuUsage );
	if( !type )
	{
		type = TEX_TYPE_2D;
	}

	auto hr = m_renderTarget.Create(
		Tr2BitmapDimensions( type, format, width, height, 1, mipLevelCount ),
		Tr2MsaaDesc( msaaType, msaaQuality ),
		gpuUsage,
		cpuUsage,
		nullptr,
		renderContext ).GetResult();
	if( SUCCEEDED( hr ) )
	{
		m_width = width;
		m_height = height;
		m_mipCount = mipLevelCount;
		m_format = format;
		m_msaa = Tr2MsaaDesc( msaaType, msaaQuality );
		m_flags = ExFlag( flags );
		m_type = type;
		m_cpuUsage = cpuUsage;
		m_gpuUsage = gpuUsage;
		m_renderTarget.SetName( m_name.c_str() );
	}
	else
	{
		Destroy();
	}
	return hr;
}

int32_t Tr2RenderTarget::CreateManual(
	unsigned width, 
	unsigned height, 
	unsigned mipLevelCount, 
	Tr2RenderContextEnum::PixelFormat format,
	unsigned msaaType,
	unsigned msaaQuality,
	Tr2RenderContextEnum::ExFlag flags,
	Tr2RenderContextEnum::TextureType type,
	Tr2CpuUsage::Type cpuUsage,
	Tr2GpuUsage::Type gpuUsage )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( IsAttached() )
	{
		return E_INVALIDARG;
	}

	auto hr = m_renderTarget.Create(
		Tr2BitmapDimensions( type, format, width, height, 1, mipLevelCount ),
		Tr2MsaaDesc( msaaType, msaaQuality ),
		gpuUsage,
		cpuUsage,
		nullptr,
		renderContext ).GetResult();
	if( SUCCEEDED( hr ) )
	{
		m_width = width;
		m_height = height;
		m_mipCount = mipLevelCount;
		m_format = format;
		m_msaa = Tr2MsaaDesc( msaaType, msaaQuality );
		m_flags = ExFlag( flags );
		m_type = type;
		m_cpuUsage = cpuUsage;
		m_gpuUsage = gpuUsage;
		m_renderTarget.SetName( m_name.c_str() );
	}
	else
	{
		Destroy();
	}
	return hr;
}

Tr2TextureAL* Tr2RenderTarget::GetTexture()
{
	auto& rt = GetRenderTarget();
	if( rt.IsValid() && Tr2GpuUsage::HasFlag( rt.GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE ) )
	{
		return  &rt;
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Attaches this object to existing AL render target. Attached Tr2RenderTarget doesn't
//   own the AL object, but only references it. Used for attaching to special render 
//   targets (swap chain back buffers).  
// Arguments:
//   renderTarget - AL render target
//   owner - Render target owner object: Tr2RenderTarget locks it so that AL renderTarget
//     is not deleted before this Tr2RenderTarget
// --------------------------------------------------------------------------------------
void Tr2RenderTarget::Attach( const Tr2TextureAL& renderTarget, IRoot* owner )
{
	Destroy();
	m_attachedRenderTarget = renderTarget;
	if( m_attachedOwner != owner )
	{
		m_attachedOwner = owner;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Detaches this object from the attached AL render target.
// --------------------------------------------------------------------------------------
void Tr2RenderTarget::Detach()
{
	m_attachedOwner = nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the referenced AL render target is attached (with Attach method) or owned.  
// Return Value:
//   true if referenced AL render target is attached
//   false if referenced AL render target is owned by this object
// --------------------------------------------------------------------------------------
bool Tr2RenderTarget::IsAttached() const
{
	return m_attachedOwner != nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns referenced render target (either owned or attached).  
// Return Value:
//   Referenced render target 
// --------------------------------------------------------------------------------------
Tr2TextureAL& Tr2RenderTarget::GetRenderTarget()
{
	if( IsAttached() )
	{
		return m_attachedRenderTarget;
	}
	return m_renderTarget;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns referenced render target (either owned or attached).  
// Return Value:
//   Referenced render target 
// --------------------------------------------------------------------------------------
const Tr2TextureAL& Tr2RenderTarget::GetRenderTarget() const
{
	if( IsAttached() )
	{
		return m_attachedRenderTarget;
	}
	return m_renderTarget;
}

bool Tr2RenderTarget::IsValid() const
{
	return GetRenderTarget().IsValid();
}

void Tr2RenderTarget::Destroy()
{
	m_renderTarget = Tr2TextureAL();
	m_width = 0;
	m_height = 0;
	m_mipCount = 0;
	m_format = PIXEL_FORMAT_UNKNOWN;
	m_msaa = Tr2MsaaDesc();
	m_flags = EX_NONE;
	m_type = TEX_TYPE_INVALID;
	m_gpuUsage = Tr2GpuUsage::NONE;
	m_cpuUsage = Tr2CpuUsage::NONE;
}

bool Tr2RenderTarget::IsReadable() const
{
	return GetRenderTarget().IsValid() && Tr2GpuUsage::HasFlag( GetRenderTarget().GetGpuUsage(), Tr2GpuUsage::SHADER_RESOURCE );
}

long Tr2RenderTarget::GenerateMipMaps()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return GetRenderTarget().GenerateMipMaps( renderContext ).GetResult();
}

long Tr2RenderTarget::Resolve( Tr2RenderTarget* destination )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !destination )
	{
		return E_FAIL;
	}
	return GetRenderTarget().Resolve( *destination, renderContext ).GetResult();
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
bool Tr2RenderTarget::HasALObject( int type, size_t object )
{
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns DirectX shared handle of contained AL render target.
// Return Value:
//   Shared handle of contained AL render target
// --------------------------------------------------------------------------------------
uintptr_t Tr2RenderTarget::GetSharedHandle() const
{
	return m_renderTarget.GetSharedHandle();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns render target width in pixels.
// Return Value:
//   Render target width in pixels
// --------------------------------------------------------------------------------------
uint32_t Tr2RenderTarget::GetWidth() const
{
	return GetRenderTarget().GetWidth();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns render target height in pixels.
// Return Value:
//   Render target height in pixels
// --------------------------------------------------------------------------------------
uint32_t Tr2RenderTarget::GetHeight() const
{
	return GetRenderTarget().GetHeight();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns number of mip levels in the render target.
// Return Value:
//   Number of mip levels in the render target
// --------------------------------------------------------------------------------------
uint32_t Tr2RenderTarget::GetMipCount() const
{
	return GetRenderTarget().GetMipCount();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns number of samples in MSAA render target.
// Return Value:
//   Number of samples in MSAA render target
// --------------------------------------------------------------------------------------
uint32_t Tr2RenderTarget::GetMsaaType() const
{
	return GetRenderTarget().GetMsaaDesc().samples;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns MSAA quality of the render target.
// Return Value:
//   MSAA quality of the render target
// --------------------------------------------------------------------------------------
uint32_t Tr2RenderTarget::GetMsaaQuality() const
{
	return GetRenderTarget().GetMsaaDesc().quality;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns render target pixel format.
// Return Value:
//   Render target pixel format
// --------------------------------------------------------------------------------------
Tr2RenderContextEnum::PixelFormat Tr2RenderTarget::GetFormat() const
{
	return GetRenderTarget().GetFormat();
}

Tr2RenderContextEnum::TextureType Tr2RenderTarget::GetType() const
{
	return GetRenderTarget().GetType();
}

// --------------------------------------------------------------------------------------
void Tr2RenderTarget::ReleaseResources( TriStorage s )
{
	if( m_renderTarget.IsValid() && ( m_renderTarget.GetMemoryClass() & s ) )
	{
		m_renderTarget = Tr2TextureAL();
	}
}

// --------------------------------------------------------------------------------------
bool Tr2RenderTarget::OnPrepareResources()
{
	if( !m_renderTarget.IsValid() && m_width != 0 )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		m_renderTarget.Create( Tr2BitmapDimensions( m_type, m_format, m_width, m_height, 1, m_mipCount ), m_msaa, m_gpuUsage, m_cpuUsage, nullptr, renderContext );
	}
	return true;
}
