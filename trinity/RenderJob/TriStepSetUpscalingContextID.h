// Copyright © 2024 CCP ehf.

#pragma once

#include "TriRenderStep.h"

BLUE_CLASS( TriStepSetUpscalingContextID ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepSetUpscalingContextID( IRoot* lockobj = 0 );
	~TriStepSetUpscalingContextID( void );

	void py__init__( Be::OptionalWithDefaultValue<uint32_t, Tr2UpscalingAL::INVALID_CONTEXT_ID> upscalingContextID = Tr2UpscalingAL::INVALID_CONTEXT_ID );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

private:
	uint32_t m_upscalingContextID;
};

TYPEDEF_BLUECLASS( TriStepSetUpscalingContextID );
