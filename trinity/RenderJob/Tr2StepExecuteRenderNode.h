// Copyright © 2026 CCP ehf.

#pragma once

#include "TriRenderStep.h"

BLUE_DECLARE_INTERFACE( ITr2RenderNode );


BLUE_CLASS( Tr2StepExecuteRenderNode ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext ) override;

	Tr2RenderTargetPtr m_destinationTarget;
	ITr2RenderNodePtr m_node;
	bool m_clearTargetOnFailure = true;

private:
	void ClearOnFailure( Tr2RenderContext & renderContext );

	bool m_failedLastTime = false;
};

TYPEDEF_BLUECLASS( Tr2StepExecuteRenderNode );
