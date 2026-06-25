// Copyright © 2020 CCP ehf.

#pragma once

#include "Tr2AudioStretchBase.h"
#include <ITr2Audio.h>

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
