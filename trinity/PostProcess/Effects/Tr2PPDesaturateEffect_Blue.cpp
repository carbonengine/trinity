// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPDesaturateEffect.h"

BLUE_DEFINE( Tr2PPDesaturateEffect );

const Be::ClassInfo* Tr2PPDesaturateEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPDesaturateEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "intensity", m_intensity, "The intensity of the saturation", Be::READWRITE | Be::PERSIST )


	EXPOSURE_CHAINTO( Tr2PPEffect )
}
