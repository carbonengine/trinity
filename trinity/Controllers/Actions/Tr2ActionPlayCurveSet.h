// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"
#include "Include/ITr2Updateable.h"


BLUE_CLASS( Tr2ActionPlayCurveSet ) :
	public ITr2ControllerAction, public ITr2Updateable
{
public:
	Tr2ActionPlayCurveSet( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;
	void Stop( ITr2ActionController & controller ) override;
	void RebaseSimTime( Be::Time diff ) override;

	bool CanTransition() const override;

	virtual void Update( Be::Time realTime, Be::Time simTime );

private:
	std::string m_curveSetName;
	std::string m_rangeName;

	Be::Time m_startTime;
	Be::Time m_prevTime;
	float m_duration;
	bool m_syncToRange;
};

TYPEDEF_BLUECLASS( Tr2ActionPlayCurveSet );
