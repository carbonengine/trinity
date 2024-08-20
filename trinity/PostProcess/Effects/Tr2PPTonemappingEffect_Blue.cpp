////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTonemappingEffect.h"

BLUE_DEFINE( Tr2PPTonemappingEffect );

const Be::ClassInfo* Tr2PPTonemappingEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPTonemappingEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )
		MAP_ATTRIBUTE( "shoulderStrength", m_shoulderStrength, " ", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linearStrength", m_linearStrength, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linearAngle", m_linearAngle, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toeStrength", m_toeStrength, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toeNumerator", m_toeNumerator, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toeDenominator", m_toeDenominator, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "whiteScale", m_whiteScale, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_CHAINTO( Tr2PPEffect )
}

