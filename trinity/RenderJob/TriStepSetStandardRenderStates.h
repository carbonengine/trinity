// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepSetStandardRenderStates_h
#define TriStepSetStandardRenderStates_h


#include "TriRenderStep.h"
#include "Shader/Tr2EffectStateManager.h"

BLUE_DECLARE( TriStepSetStdRndStates );

// Note: This class should be renamed once we've lifted the 24 character
// limit on class names
class TriStepSetStdRndStates : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	TriStepSetStdRndStates( IRoot* lockobj = NULL );

	void py__init__( Be::Optional<unsigned> state );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void SetState( unsigned int state );

private:
	Tr2EffectStateManager::RenderingMode m_renderingMode;
};

TYPEDEF_BLUECLASS( TriStepSetStdRndStates );
#endif //TriStepSetStandardRenderStates_h
