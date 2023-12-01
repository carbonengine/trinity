#pragma once
#ifndef _TRISTEPREMOTEUPDATE_H_
#define _TRISTEPREMOTEUPDATE_H_


#include "TriRenderStep.h"
#include "TriView.h"
#include "TriViewport.h"
#include "TriProjection.h"

BLUE_CLASS( TriStepRemoteUpdate ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRemoteUpdate( IRoot* lockobj = 0 );
	~TriStepRemoteUpdate(void);

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	// Python __init__ constructor
	void py__init__( 
		TriView* view, 
		TriProjection* projection, 
		TriViewport* viewport, 
		Be::OptionalWithDefaultValue<int, -1> id );

	void SetData( TriView* view, TriProjection *proj, TriViewport* viewport, int id );
private:
	bool OpenSharedMemoryAndEvents();

	TriViewPtr m_view;
	TriViewportPtr m_viewport;
	TriProjectionPtr m_projection;

	HANDLE									m_sharedMemoryHandle;
	HANDLE									m_needsHndl;
	HANDLE									m_readingHndl;
	HANDLE									m_writingHndl;
	const char*								m_sharedMemory;
	int										m_id;
};

TYPEDEF_BLUECLASS( TriStepRemoteUpdate );

#endif