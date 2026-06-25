// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetDepthStencil.h"
#include "Tr2Renderer.h"

#include "Tr2DepthStencil.h"


TriStepSetDepthStencil::TriStepSetDepthStencil( IRoot* lockobj )
{
}

TriStepSetDepthStencil::~TriStepSetDepthStencil( void )
{
}

TriStepResult TriStepSetDepthStencil::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_depthStencil )
	{
		return renderContext.m_esm.SetDepthStencilBuffer( *m_depthStencil ) ? RS_OK : RS_FAILED;
	}

	return renderContext.m_esm.SetDepthStencilBuffer( Tr2TextureAL() ) ? RS_OK : RS_FAILED;
}