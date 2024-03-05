#pragma once
#ifndef _TRISTEPSETDEPTHSTENCIL_H_
#define _TRISTEPSETDEPTHSTENCIL_H_


#include "TriRenderStep.h"

BLUE_DECLARE( Tr2DepthStencil );

BLUE_CLASS( TriStepSetDepthStencil ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepSetDepthStencil( IRoot* lockobj = 0);
	~TriStepSetDepthStencil(void);

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	// Python __init__ constructor
	void SetDepthStencil( Tr2DepthStencil* depthStencil ) { m_depthStencil = depthStencil; }

private:
	Tr2DepthStencilPtr	m_depthStencil;
};

TYPEDEF_BLUECLASS( TriStepSetDepthStencil );

#endif