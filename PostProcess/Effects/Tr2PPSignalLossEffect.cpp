////////////////////////////////////////////////////////////////////////////////
//
// Created:		1/15/2019
// Copyright:	CCP 2019
//

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



