////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveOccluder.h"

BLUE_DEFINE( EveOccluder );

const Be::ClassInfo* EveOccluder::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveOccluder, "" )
        MAP_INTERFACE( EveOccluder )

		MAP_ATTRIBUTE( "name", m_name, "A name for this occluder", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "sprites", m_sprites, "a list of sprites for the occlusion query", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
