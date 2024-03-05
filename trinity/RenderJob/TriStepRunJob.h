#pragma once
#ifndef TriStepRunJob_h
#define TriStepRunJob_h


#include "TriRenderStep.h"
#include "include/TriColor.h"

BLUE_DECLARE( TriRenderJob );

BLUE_CLASS( TriStepRunJob ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepRunJob( IRoot* lockobj = 0 );
	~TriStepRunJob(void);

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void SetRenderJob( TriRenderJob* job );

private:
	TriRenderJobPtr m_job;
};

TYPEDEF_BLUECLASS( TriStepRunJob );

#endif