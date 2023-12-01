#pragma once
#ifndef TriStepTestBlocking_h
#define TriStepTestBlocking_h


#include "TriRenderStep.h"
#include "include/TriColor.h"

BLUE_CLASS( TriStepTestBlocking ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepTestBlocking( IRoot* lockobj = 0 );
	~TriStepTestBlocking(void);

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

private:
	bool m_inProgress;
};

TYPEDEF_BLUECLASS( TriStepTestBlocking );

#endif