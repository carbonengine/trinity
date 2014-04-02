////////////////////////////////////////////////////////////
//
//    Created:   July 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "Tr2RenderContext.h"
#include "Tr2VariableStore.h"
#include "Tr2RenderTarget.h"

Tr2RenderContextBase::Tr2RenderContextBase( Tr2RenderContext& renderContext )
	:m_esm( renderContext )
{
	m_backBuffer.CreateInstance();
	m_backBuffer->m_name = "backbuffer";
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	m_varStore.CreateInstance();
	m_varStore->SetParentVariableStore( nullptr );	// do _not_ want magic chaining
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Called by Tr2RenderContextAL when a primary or secondary context is created. 
//   Initializes Tr2EffectStateManager instance.
// Arguments:
//   renderContext - AL render context created
// --------------------------------------------------------------------------------------
void Tr2RenderContextBase::OnContextCreated( Tr2RenderContextAL& renderContext )
{
	m_backBuffer->Attach( &renderContext.GetDefaultBackBuffer(), this );
	m_esm.Initialize();
}

// --------------------------------------------------------------------------------------
// Description:
//   Called by DX11 Tr2RenderContextAL when a texture is set as a render target. In this 
//   case DX11 runtime will unset this texture from texture sampling registers (if it's
//   bound for read). Informs Tr2EffectStateManager instance of this so that it can 
//   update its state accordingly.
// Arguments:
//   texture - Texture that is unset from reading register
//   renderContext - Unused
// --------------------------------------------------------------------------------------
void Tr2RenderContextBase::OnTextureUnset( const Tr2TextureAL& texture, Tr2RenderContextAL& )
{
	m_esm.ForgetTexture( texture );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns back buffer render target as Blue-exposed Tr2RenderTarget. Exposed to 
//   script.
// Return Value:
//   Back buffer render target 
// --------------------------------------------------------------------------------------
Tr2RenderTargetPtr Tr2RenderContextBase::GetBackBuffer()
{
	return m_backBuffer;
}

#if TRINITY_PLATFORM != TRINITY_DIRECTX11
// --------------------------------------------------------------------------------------
// Description:
//   Returns render context's local variable store. In case of DX9 and OpenGL this is the 
//   global variable store.
// Return Value:
//   Render context's local variable store
// --------------------------------------------------------------------------------------
Tr2VariableStore& Tr2RenderContextBase::GetVariableStore() 
{ 
	return GlobalStore(); 
}
#else
// --------------------------------------------------------------------------------------
// Description:
//   Returns render context's local variable store. 
// Return Value:
//   Render context's local variable store
// --------------------------------------------------------------------------------------
Tr2VariableStore& Tr2RenderContextBase::GetVariableStore() 
{ 
	return *m_varStore; 
}
#endif


Tr2RenderContext::Tr2RenderContext()
	:Tr2RenderContextBase( *this )
{
	m_events = this;
}

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
Tr2PrimaryRenderContext::Tr2PrimaryRenderContext()
	:Tr2RenderContextBase( *reinterpret_cast<Tr2RenderContext*>( this ) )
{
	m_events = this;
}

Tr2PrimaryRenderContext::operator Tr2RenderContext&()
{
	return *reinterpret_cast<Tr2RenderContext*>( this );
}
#endif


namespace {
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
	Tr2PrimaryRenderContextPtr	s_mainThreadRenderContext;
#else
	Tr2RenderContextPtr s_mainThreadRenderContext;
#endif
}
	
void Tr2RenderContext::DestroyMainThreadRenderContext()
{
	if( s_mainThreadRenderContext )
	{
		s_mainThreadRenderContext->Destroy();
		s_mainThreadRenderContext.Unlock();
		Tr2RenderContextAL::SetPrimaryRenderContext( nullptr );
	}
}

Tr2PrimaryRenderContext& Tr2RenderContext_GetMainThreadRenderContext()
{
	if( !s_mainThreadRenderContext )
	{
		s_mainThreadRenderContext.CreateInstance();
		Tr2RenderContextAL::SetPrimaryRenderContext( s_mainThreadRenderContext );
	}

	return *s_mainThreadRenderContext;
}
