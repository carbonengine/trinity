// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPSignalLossEffect.h"

BLUE_DEFINE( Tr2PPSignalLossEffect );

const Be::ClassInfo* Tr2PPSignalLossEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPSignalLossEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "strength", m_strength, "The strength of the signal loss", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
