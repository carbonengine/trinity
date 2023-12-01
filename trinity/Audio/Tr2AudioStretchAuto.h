////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//
//    Description:
// 	    Provides audio functionality to long running EveStretch effects (aka lasers).
//		This class exposes methods that EveStretch effects can use to trigger audio events.
//      This class should only be used when the timing of triggering audio events
//      is decided by EveStretch. The only configuration given to the sound designer is
//      which events to fire.

#pragma once

#include "Tr2AudioStretchBase.h"
#include "ITr2Audio.h"

BLUE_CLASS( Tr2AudioStretchAuto ) :
	public Tr2AudioStretchBase
{
public:
	EXPOSE_TO_BLUE();

	Tr2AudioStretchAuto( IRoot* lockobj = NULL );
	virtual ~Tr2AudioStretchAuto();

	unsigned int TriggerOutburstEvent();
	unsigned int TriggerImpactEvent();
	unsigned int TriggerStretchEvent();
protected:
	std::wstring m_outburstEvent;
	std::wstring m_impactEvent;
	std::wstring m_stretchEvent;
};

TYPEDEF_BLUECLASS( Tr2AudioStretchAuto );
