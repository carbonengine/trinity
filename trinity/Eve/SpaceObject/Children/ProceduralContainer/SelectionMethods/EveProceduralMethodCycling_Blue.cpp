// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodCycling.h"

BLUE_DEFINE( EveProceduralMethodCycling );

const Be::ClassInfo* EveProceduralMethodCycling::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodCycling, "" )
		MAP_INTERFACE( EveProceduralMethodCycling )
		MAP_INTERFACE( IEveProceduralSelectionMethod )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "selectedChild", m_selectedChildIndex, "selection result", Be::READ )

		MAP_ATTRIBUTE( "startTimeOffset", m_startTimeOffset, "applies to first switch after playing\n"
															 ":jessica-group:randomOptions",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomizeOrder", m_randomizeOrder, "enable this and it will always pick a random one to swap to\n"
														   ":jessica-group:randomOptions",
					   Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "debugVolumes", m_debugVolumes, "", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "restart", SelectParameter, "restart the container \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-arrows-spin\n" )

	EXPOSURE_END()
}
