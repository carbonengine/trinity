////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveLineContainer.h"

BLUE_DEFINE( EveLineContainer );

const Be::ClassInfo* EveLineContainer::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveLineContainer, "" )
        MAP_INTERFACE( EveLineContainer )
		MAP_INTERFACE( IEveSpaceObject2 )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE );
		MAP_ATTRIBUTE( "connectors", m_connectors, "", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "lineSet", m_lineSet, "", Be::READWRITE | Be::PERSIST );

    EXPOSURE_END()
}