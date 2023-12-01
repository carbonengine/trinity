#include "StdAfx.h"
#include "TriObserverLocal.h"

BLUE_DEFINE( TriObserverLocal );
BLUE_DEFINE_INTERFACE( ITriObserverLocal );

const Be::ClassInfo* TriObserverLocal::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriObserverLocal, "Connects locators to placement observers." )
		MAP_INTERFACE( ITriObserverLocal )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "front", m_front, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "observer", m_observer, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}