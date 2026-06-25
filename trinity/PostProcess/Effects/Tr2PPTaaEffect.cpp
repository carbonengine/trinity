// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"


Tr2PPTaaEffect::Tr2PPTaaEffect( IRoot* lockobj ) :
	m_quality( Tr2PPTaaEffect::Quality::TAA_HIGH ),
	m_debugMode( Tr2PPTaaEffect::Debug::TAA_DEBUG_OFF ),
	m_earlyOutThreshold( 0.001 )
{
}

Tr2PPTaaEffect::~Tr2PPTaaEffect()
{
}
