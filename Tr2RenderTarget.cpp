#include "StdAfx.h"
#include "Tr2RenderTarget.h"

using namespace Tr2RenderContextEnum;

Tr2RenderTarget::Tr2RenderTarget( IRoot* )
	:m_attachedRenderTarget( nullptr )
{	
}

Tr2RenderTarget::~Tr2RenderTarget()
{
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
	Tr2RenderContextEnum::ExFlag flags )
{
	if( width && height && format )
	{
		CCP_ASSERT( msaaType <= 1 || mipCount <= 1 );	// can't have msaa and mips at the same time.
		Create( width, height, mipCount, format, msaaType, msaaQuality, flags );
	}		
}

long Tr2RenderTarget::Create(	
	unsigned width, 
	unsigned height, 
	unsigned mipLevelCount, 
	Tr2RenderContextEnum::PixelFormat format,
	unsigned msaaType,
	unsigned msaaQuality,
	Tr2RenderContextEnum::ExFlag flags )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( IsAttached() )
	{
		return E_INVALIDARG;
	}
	return m_renderTarget.Create(
		width,
		height,
		mipLevelCount,
		format,
		Tr2MsaaDesc( msaaType, msaaQuality ),
		( flags & Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS ) ? Tr2RenderContextEnum::USAGE_UNORDERED_ACCESS : 0,
		ExFlag( flags ),
		renderContext ).GetResult();
}

Tr2TextureAL* Tr2RenderTarget::GetTexture()
{
	auto& rt = GetRenderTarget();
	if( rt.IsValid() && rt.GetTexture().IsValid() )
	{
		return  &rt.GetTexture();
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
void Tr2RenderTarget::Attach( Tr2RenderTargetAL* renderTarget, IRoot* owner )
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
Tr2RenderTargetAL& Tr2RenderTarget::GetRenderTarget()
{
	if( IsAttached() )
	{
		return *m_attachedRenderTarget;
	}
	return m_renderTarget;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns referenced render target (either owned or attached).  
// Return Value:
//   Referenced render target 
// --------------------------------------------------------------------------------------
const Tr2RenderTargetAL& Tr2RenderTarget::GetRenderTarget() const
{
	if( IsAttached() )
	{
		return *m_attachedRenderTarget;
	}
	return m_renderTarget;
}

bool Tr2RenderTarget::IsValid() const
{
	return GetRenderTarget().IsValid();
}

void Tr2RenderTarget::Destroy()
{
	m_renderTarget.Destroy();
}

bool Tr2RenderTarget::IsReadable() const
{
	return GetRenderTarget().GetTexture().IsValid() && GetRenderTarget().GetMsaaDesc().samples < 2;
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
	if( type == OT_RENDER_TARGET )
	{
		return GetRenderTarget() == *reinterpret_cast<Tr2RenderTargetAL*>( object );
	}
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