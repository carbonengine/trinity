// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPresentSwapChain.h"

TriStepPresentSwapChain::TriStepPresentSwapChain( IRoot* lockobj )
{
}

TriStepResult TriStepPresentSwapChain::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_swapChain )
	{
		m_swapChain->Present( renderContext );
	}
	return RS_OK;
}

void TriStepPresentSwapChain::py__init__( Tr2SwapChain* sc )
{
	m_swapChain = sc;
}
