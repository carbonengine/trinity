// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "Tr2AudioStretchAuto.h"
#include "Tr2Renderer.h"

Tr2AudioStretchAuto::Tr2AudioStretchAuto( IRoot* lockobj )
{
}

Tr2AudioStretchAuto::~Tr2AudioStretchAuto()
{
}

unsigned int Tr2AudioStretchAuto::TriggerOutburstEvent()
{
	if( nullptr != m_sourceEmitter )
	{
		return m_sourceEmitter.p->SendEvent( m_outburstEvent );
	}
	return 0;
}

unsigned int Tr2AudioStretchAuto::TriggerImpactEvent()
{
	if( nullptr != m_destEmitter )
	{
		return m_destEmitter.p->SendEvent( m_impactEvent );
	}
	return 0;
}

unsigned int Tr2AudioStretchAuto::TriggerStretchEvent()
{
	if( nullptr != m_stretchEmitter )
	{
		return m_stretchEmitter.p->SendEvent( m_stretchEvent );
	}
	return 0;
}