// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveTacticalTrails.h"
#include <ITriFunction.h>


BLUE_DEFINE( EveTacticalTrails );
const Be::ClassInfo* EveTacticalTrails::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTacticalTrails, "" )
		MAP_INTERFACE( EveTacticalTrails )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "fadeOutTime", m_fadeOutTime, "Time in seconds for the trail to fully fade out", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailEffect", m_trailEffect, "The effect used to render the trail", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "egoBall", m_egoBall, "Player ship ball", Be::READWRITE )

		MAP_ATTRIBUTE( "segments", m_segmentCount, "Number of segments rendered, for debugging", Be::READ )

		MAP_METHOD_AND_WRAP(
			"RegisterObject",
			RegisterObject,
			"Registers a new object with the trail system. After registering the object will leave trails when it moves.\n"
			"The object is supposed to be a Destiny ball for a ship.\n\n"
			":param obj: The object to register\n" )
		MAP_METHOD_AND_WRAP(
			"UnregisterObject",
			UnregisterObject,
			"Unregisters a previously registered object from the trails system. The trails left by the object prior to\n"
			"unregistering will fade out over time.\n\n"
			":param obj: The object to unregister\n" )
	EXPOSURE_END()
}