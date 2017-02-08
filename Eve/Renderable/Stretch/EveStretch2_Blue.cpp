////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "EveStretch2.h"


BLUE_DEFINE( EveStretch2 );

const Be::ClassInfo* EveStretch2::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveStretch2, "" )
		MAP_INTERFACE( EveStretch2 )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IEveFiringEffectElement )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		(
			"effect",
			m_effect,
			"Effect to use for the stretch",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"quadCount",
			m_quadCount,
			"Number of quads to render for the effect",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"start",
			m_start,
			"Curve set to play when firing effect starts",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"loop",
			m_loop,
			"Curve set that is played when the effect is active",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"end",
			m_end,
			"Curve set to play when the effect stops",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"sourceLight",
			m_sourceLight,
			"Point light at the effect source",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"destinationLight",
			m_destinationLight,
			"Point light at the effect destination",
			Be::READWRITE | Be::PERSIST
		)

	EXPOSURE_END()
}
