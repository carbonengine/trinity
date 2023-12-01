#include "StdAfx.h"
#include "EveLocator2.h"
#include "TriConstants.h"

BLUE_DEFINE( EveLocator2 );

const Be::ClassInfo* EveLocator2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveLocator2, "" )
        MAP_INTERFACE( EveLocator2 )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_transform, "", Be::READWRITE | Be::PERSIST	)

    EXPOSURE_END()
}
