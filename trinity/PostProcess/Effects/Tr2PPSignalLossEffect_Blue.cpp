////////////////////////////////////////////////////////////////////////////////
//
// Created:		1/15/2019 
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPSignalLossEffect.h"

BLUE_DEFINE( Tr2PPSignalLossEffect );

const Be::ClassInfo* Tr2PPSignalLossEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPSignalLossEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "strength", m_strength, "The strength of the signal loss", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
	EXPOSURE_CHAINTO( Tr2PPEffect )

}

