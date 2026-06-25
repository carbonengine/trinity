// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetStandardRenderStates.h"
#include "Shader/Tr2EffectStateManager.h"

TriStepSetStdRndStates::TriStepSetStdRndStates( IRoot* lockobj ) :
	m_renderingMode( Tr2EffectStateManager::RM_OPAQUE )
{
}

TriStepResult TriStepSetStdRndStates::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyStandardStates( m_renderingMode );
	return RS_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
void TriStepSetStdRndStates::py__init__( Be::Optional<unsigned> state )
{
	if( state.IsAssigned() )
	{
		SetState( state );
	}
}

void TriStepSetStdRndStates::SetState( unsigned int rm )
{
	if( rm < Tr2EffectStateManager::RM_COUNT )
	{
		m_renderingMode = (Tr2EffectStateManager::RenderingMode)rm;
	}
}