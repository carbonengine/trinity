////////////////////////////////////////////////////////////////////////////////
//
// Created:		1/15/2019 
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPGodRaysEffect.h"

BLUE_DEFINE( Tr2PPGodRaysEffect );

const Be::ClassInfo* Tr2PPGodRaysEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPGodRaysEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "godRayColor", m_godRayColor, "The color of the godrays", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "intensity", m_intensity, "The intensity of the godrays", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseTexturePath", m_noiseTexturePath, "The noise texture to use", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_CHAINTO( Tr2PPEffect )

}

