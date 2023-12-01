#pragma once

#ifndef Tr2RenderJobs_h_
#define Tr2RenderJobs_h_


#include "Tr2DeviceResource.h"

BLUE_DECLARE( TriRenderJob );
BLUE_DECLARE_VECTOR( TriRenderJob );

BLUE_CLASS( Tr2RenderJobs ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2RenderJobs( IRoot* lockobj = 0 );
	~Tr2RenderJobs();
	
	void Run( Be::Time realTime, Be::Time simTime );
	void RunUpdate( Be::Time realTime, Be::Time simTime );
	
	PTriRenderJobVector m_scheduledRecurring;
	PTriRenderJobVector m_scheduledOnce;
	PTriRenderJobVector m_scheduledChained;
	PTriRenderJobVector m_updateRecurring;
};

TYPEDEF_BLUECLASS( Tr2RenderJobs )
BLUE_DECLARE_VECTOR( Tr2RenderJobs )

#endif	// Tr2RenderJobs_h_
