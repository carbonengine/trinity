#include "StdAfx.h"
#include "EveStation2.h"

BLUE_DEFINE( EveStation2 );

const Be::ClassInfo* EveStation2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveStation2, "" )
        MAP_INTERFACE( EveStation2 )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "hologramSets", m_hologramSets, "All the billboards, holograms, etc.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "environmentSpriteSets", m_environmentSpriteSets, "All the extended blinkies, for landstrips, etc.", Be::READWRITE | Be::PERSIST )

    EXPOSURE_CHAINTO( EveSpaceObject2 )
}