#include "StdAfx.h"
#include "BackAndForth.h"

BLUE_DEFINE( BackAndForth );

const Be::ClassInfo* BackAndForth::ExposeToBlue()
{
	EXPOSURE_BEGIN( BackAndForth, "" )
		MAP_INTERFACE( BackAndForth )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "arrivedRadius", m_arrivedRadius, ":jessica-group: BackAndForth", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "slowDownRadius", m_slowDownRadius, ":jessica-group: BackAndForth", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "backAndForthWeight", m_backAndForthWeight, ":jessica-group: BackAndForth", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fxBehavior", m_fxBehavior, ":jessica-group: BackAndForth", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSet", m_locatorSets, "", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "AddLocatorSet", AddLocatorSet, "Adds a locatorSet to the behavior \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-map-pin\n" )

	EXPOSURE_END()
}