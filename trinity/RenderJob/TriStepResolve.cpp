// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepResolve.h"

#include "Tr2RenderTarget.h"

TriStepResolve::TriStepResolve( IRoot* lockobj ) :
	m_generateMipmap( false )
{
}

TriStepResult TriStepResolve::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_source || !m_destination || !m_source->GetRenderTarget().IsValid() || !m_destination->GetRenderTarget().IsValid() )
	{
		return RS_OK;
	}

	if( FAILED( m_source->GetRenderTarget().Resolve( *m_destination, renderContext ) ) )
	{
		return RS_FAILED;
	}

	if( m_generateMipmap )
	{
		m_destination->GetRenderTarget().GenerateMipMaps( renderContext );
	}

	return RS_OK;
}

void TriStepResolve::py__init__( Tr2RenderTarget* destination, Tr2RenderTarget* source )
{
	m_destination = destination;
	m_source = source;
}
