////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2AudioStretchAuto.h"

BLUE_DEFINE( Tr2AudioStretchAuto );

const Be::ClassInfo* Tr2AudioStretchAuto::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2AudioStretchAuto, ":jessica-help-url: https://wiki.ccpgames.com/display/CAudio/Tr2AudioStretchAuto" )
		MAP_INTERFACE( Tr2AudioStretchBase )
        MAP_INTERFACE( ITr2Audio )

		MAP_ATTRIBUTE(
			"sourceEmitter",
			m_sourceEmitter,
			"Source audio emitter, Follows the source position. The outburst event will come from this emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"destinationEmitter",
			m_destEmitter,
			"Destination audio emitter, follows the destination position. The impact event will come from this emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"stretchEmitter",
			m_stretchEmitter,
			"Audio emitter for the looping part of the stretch effect. It follows the camera as long as it is between the "
			"source and destination. The stretch event will come from this emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"outburstEvent",
			m_outburstEvent,
			"The event to play on the outburst of the weapon firing. Comes from the source audio emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"impactEvent",
			m_impactEvent,
			"The event to play on the impact of the weapon hitting the destination. Comes from the destination audio emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"stretchEvent",
			m_stretchEvent,
			"The event to play on the stretch effect between the source and destination. Comes from the stretch emitter.",
			Be::READWRITE | Be::PERSIST
		)

	EXPOSURE_END()
}
