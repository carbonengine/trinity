// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPushDepthStencil.h"
#include "Tr2Renderer.h"

#include "Tr2DepthStencil.h"

TriStepPushDepthStencil::TriStepPushDepthStencil( IRoot* lockobj ) :
	m_pushCurrent( false ) // need this flag to know the difference between pushing nothing, and pushing None to disable depthStencil
{
}

TriStepPushDepthStencil::~TriStepPushDepthStencil( void )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// Arguments:
//   depthStencil - Initial value of depth stencil step attribute
// --------------------------------------------------------------------------------------
void TriStepPushDepthStencil::py__init__( Be::Optional<Tr2DepthStencil*> depthStencil )
{
	if( depthStencil.IsAssigned() )
	{
		m_depthStencil = depthStencil;
	}
	else
	{
		m_pushCurrent = true;
	}
}

TriStepResult TriStepPushDepthStencil::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_depthStencil )
	{
		return renderContext.m_esm.PushDepthStencilBuffer( *m_depthStencil ) ? RS_OK : RS_FAILED;
	}
	else if( m_pushCurrent )
	{
		renderContext.m_esm.PushDepthStencilBuffer();
		return RS_OK;
	}

	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );
	return RS_OK;
}