#pragma once

#ifndef _TRIRENDERJOB_H_
#define _TRIRENDERJOB_H_


#include "Tr2DeviceResource.h"

BLUE_DECLARE( TriRenderStep );
BLUE_DECLARE_VECTOR( TriRenderStep );

enum TriRenderJobStatus
{
	RJ_INIT,
	RJ_IN_PROGRESS,
	RJ_DONE,
	RJ_FAILED
};

BLUE_CLASS( TriRenderJob ):
	public IRoot,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	TriRenderJob( IRoot* lockobj = 0 );
	~TriRenderJob(void);

	TriRenderJobStatus Run( Be::Time realTime, Be::Time simTime, Tr2RenderContext *renderContext = nullptr );

	TriRenderStepVector& Steps() { return m_renderSteps; }

	// ITriDeviceResource is needed, for cases when people decorate the renderjob class
	// as a manager for the surfaces and resource that are needed for it
	virtual void ReleaseResources( TriStorage s );
private:
	virtual bool OnPrepareResources();

private:
	std::string m_name;
	PTriRenderStepVector m_renderSteps;
	TriRenderJobStatus m_status;

	// keep an index instead of an iterator, in case we yield during execution and
	// modify the list at the same time.  What that is supposed to do is debatable,
	// but at least we don't want to crash pointing at bad memory.
	size_t	m_currentStep;

	// Check for RT/DS stack depth before and after RJ is run and
	// assert/repair stack when needed.
	bool m_stackGuard;

	// The render job is only run when enabled - this is primarily for debugging
	// and performance analysis purposes
	bool m_enabled;
};

TYPEDEF_BLUECLASS( TriRenderJob )
BLUE_DECLARE_VECTOR( TriRenderJob )

#endif