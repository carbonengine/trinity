////////////////////////////////////////////////////////////
//
//    Created:   November 2017
//    Copyright: CCP 2017
//
#include "StdAfx.h"
#include "EveHazeSet.h"

BLUE_DEFINE( EveHazeSet );

const Be::ClassInfo* EveHazeSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveHazeSet, "" )
		MAP_INTERFACE( EveHazeSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IEveSpaceObjectAttachment )

		MAP_ATTRIBUTE( "display", m_display, "Specifies whether to render this set or not", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "Standard name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hazes", m_hazes, "The list of all haze items", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "effect", m_effect, "Effect to use for rendering hazes", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild resources after adding/removing/changing individual items" )

	EXPOSURE_END()
}
