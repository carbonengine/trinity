// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPFadeEffect.h"

BLUE_DEFINE( Tr2PPFadeEffect );

const Be::ClassInfo* Tr2PPFadeEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPFadeEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "intensity", m_intensity, "The intensity of the fade", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "The color of the fade", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
