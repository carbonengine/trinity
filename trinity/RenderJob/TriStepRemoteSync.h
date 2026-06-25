// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepRemoteSync_h
#define TriStepRemoteSync_h

#include "TriRenderStep.h"

BLUE_CLASS( TriStepRemoteSync ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	TriStepRemoteSync( IRoot* lockobj = NULL );
	~TriStepRemoteSync();

	void py__init__( Be::OptionalWithDefaultValue<int, -1> id );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// an id to create access a unique instance of the events
	int GetId()
	{
		return m_id;
	}
	void SetId( int id )
	{
		m_id = id;
	}

private:
	HANDLE m_begin;
	HANDLE m_end;
	HANDLE m_init;
	int m_id;
};

TYPEDEF_BLUECLASS( TriStepRemoteSync );
#endif //TriStepRemoteSync_h
