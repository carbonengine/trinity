// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2StepExecuteRenderNode.h"
#include "../Tr2RenderTarget.h"
#include "../ITr2RenderNode.h"


TriStepResult Tr2StepExecuteRenderNode::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( !m_destinationTarget || !m_destinationTarget->IsValid() )
	{
		if( !m_failedLastTime )
		{
			CCP_LOGERR( "Tr2StepExecuteRenderNode::Execute called without a valid destination target" );
		}
		m_failedLastTime = true;
		return RS_FAILED;
	}
	if( !m_node )
	{
		if( !m_failedLastTime )
		{
			CCP_LOGWARN( "Tr2StepExecuteRenderNode::Execute called without a valid render node" );
		}
		m_failedLastTime = true;
		if( m_clearTargetOnFailure )
		{
			ClearOnFailure( renderContext );
			return RS_OK;
		}
		return RS_FAILED;
	}

	if( !m_node->Validate( { &m_destinationTarget->GetRenderTarget().GetDesc(), 1 }, {}, realTime, simTime ) )
	{
		if( !m_failedLastTime )
		{
			CCP_LOGWARN( "Tr2StepExecuteRenderNode::Execute render node validation failed" );
		}
		m_failedLastTime = true;
		if( m_clearTargetOnFailure )
		{
			ClearOnFailure( renderContext );
			return RS_OK;
		}
		return RS_FAILED;
	}
	m_node->Execute( { &m_destinationTarget->GetRenderTarget(), 1 }, {}, realTime, simTime, m_timer, renderContext );
	m_failedLastTime = false;
	return RS_OK;
}

void Tr2StepExecuteRenderNode::ClearOnFailure( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.SetRenderTarget( 0, *m_destinationTarget );
	renderContext.m_esm.SetDepthStencilBuffer( {} );
	renderContext.m_esm.SetFullScreenViewport();

	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, Color( 0.f, 0.f, 0.f, 1.f ), 0 );
}