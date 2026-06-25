// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPSignalLossEffect.h"


Tr2PPSignalLossEffect::Tr2PPSignalLossEffect( IRoot* lockobj ) :
	m_strength( 0.0f )
{
}


Tr2PPSignalLossEffect::~Tr2PPSignalLossEffect()
{
}

bool Tr2PPSignalLossEffect::IsActive()
{
	return m_display && m_strength > 0.0f;
}
