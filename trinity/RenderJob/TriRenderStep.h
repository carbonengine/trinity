// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRIRENDERSTEP_H_
#define _TRIRENDERSTEP_H_

#include "Tr2ProfileTimer.h"

enum TriStepResult
{
	RS_OK,
	RS_FAILED,
	RS_IN_PROGRESS,
	RS_TERMINATE
};

class Tr2RenderContext;

BLUE_CLASS( TriRenderStep ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	TriRenderStep( IRoot* lockobj = NULL );
	virtual ~TriRenderStep();

	bool IsEnabled() const;
	virtual TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext ) = 0;

	void BeginExecute( Tr2RenderContext & renderContext );
	void EndExecute( Tr2RenderContext & renderContext );

protected:
	bool GetCaptureGpuTime() const;
	void SetCaptureGpuTime( bool capture );
	bool GetCaptureCpuTime() const;
	void SetCaptureCpuTime( bool capture );
	float GpuTime() const;
	float CpuTime() const;
	const std::string& GetStatName() const;
	void SetStatName( const char* name );

	std::string m_name;
	Tr2ProfileTimer m_timer;

	// Enabled/disabled flag: disabled steps are not executed
	bool m_enabled;
};

BLUE_DECLARE_VECTOR( TriRenderStep );
TYPEDEF_BLUECLASS( TriRenderStep );

#endif