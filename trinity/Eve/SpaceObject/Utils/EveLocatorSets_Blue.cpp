////////////////////////////////////////////////////////////
//
//    Created:   February 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "EveLocatorSets.h"

BLUE_DEFINE( EveLocatorSets );

const Be::ClassInfo* EveLocatorSets::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveLocatorSets, "" )
        MAP_INTERFACE( EveLocatorSets )

		MAP_ATTRIBUTE( "name", m_name, "The unique name of this set of locators", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locators", m_locators, "List of all the locators of this set", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}