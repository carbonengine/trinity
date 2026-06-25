// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPLutEffect.h"

BLUE_DEFINE( Tr2PPLutEffect );

const Be::ClassInfo* Tr2PPLutEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPLutEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "influence", m_influence, "The influence of the LUT", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "path", m_path, "The path of the LUT\n:jessica-widget: texturepath", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
