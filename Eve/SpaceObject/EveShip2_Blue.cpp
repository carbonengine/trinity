#include "StdAfx.h"
#include "EveShip2.h"

BLUE_DEFINE( EveShip2 );

const Be::ClassInfo* EveShip2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveShip2, "" )
        MAP_INTERFACE( EveShip2 )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "speed", m_speed, "The speed of the ship", Be::READ )
		MAP_ATTRIBUTE( "maxSpeed", m_maxSpeed, "The maximum speed of the ship", Be::READWRITE )
		MAP_PROPERTY( "audioSpeedParameter", GetAudioParameter, SetAudioParameter, "The audio parameter related to the ship speed" )

		MAP_ATTRIBUTE( "boosters", m_boosters, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "RebuildBoosterSet", RebuildBoosterSet, "Call this function after adding/removing boosters" )

	EXPOSURE_CHAINTO( EveMobile )
}
