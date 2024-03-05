////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPBloomEffect.h"

BLUE_DEFINE( Tr2PPBloomEffect );

const Be::ClassInfo* Tr2PPBloomEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPBloomEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "luminanceThreshold", m_luminanceThreshold, "The threshold of the luminance", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "luminanceScale", m_luminanceScale, "The scale of the luminance", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightness", m_bloomBrightness, "The bloom brightness", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "exposureDependency", m_exposureDependency, "The exposure dependency", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "grimeWeight", m_grimeWeight, "The grime weight", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "grimePath", m_grimePath, "The grime path", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
	EXPOSURE_CHAINTO( Tr2PPEffect )


}

