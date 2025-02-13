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
		MAP_ATTRIBUTE( "slope", m_slope, ":jessica-numeric-range: (0.0, 3.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toe", m_toe, ":jessica-numeric-range: (0.0, 1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shoulder", m_shoulder, ":jessica-numeric-range: (0.0, 1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blackClip", m_blackClip, ":jessica-numeric-range: (0.0, 1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "whiteClip", m_whiteClip, ":jessica-numeric-range: (0.0, 1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scale", m_scale, ":jessica-numeric-range: (0.0, 3.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blueCorrection", m_blueCorrection, ":jessica-numeric-range: (0.0, 1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useSweeteners", m_useSweeteners, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_CHAINTO( Tr2PPEffect )
}