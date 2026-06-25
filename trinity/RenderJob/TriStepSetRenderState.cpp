// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetRenderState.h"


TriStepSetRenderState::TriStepSetRenderState( IRoot* lockobj ) :
	m_state( static_cast<Tr2RenderContextEnum::RenderState>( 0 ) ),
	m_value( 0 )
{
}

TriStepResult TriStepSetRenderState::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.SetRenderState( m_state, m_value );
	return RS_OK;
}

void TriStepSetRenderState::SetStateAndValue( unsigned int state, unsigned int value )
{
	m_state = static_cast<Tr2RenderContextEnum::RenderState>( state );
	m_value = value;
}