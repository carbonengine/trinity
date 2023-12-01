////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "EveFiringEffectElementContainer.h"

BLUE_DEFINE( EveFiringEffectElementContainer );

const Be::ClassInfo* EveFiringEffectElementContainer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveFiringEffectElementContainer, "" )
		MAP_INTERFACE( EveFiringEffectElementContainer )
		MAP_INTERFACE( IEveSpaceObject2 )

		MAP_ATTRIBUTE
		(
			"display",
			m_display,
			"Show/hide the effect",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"element",
			m_element,
			"Firing element",
			Be::READWRITE | Be::PERSIST
		)

		MAP_PROPERTY
		(
			"active",
			GetActive,
			SetActive,
			"Is the effect active"
		)

		MAP_ATTRIBUTE
		(
			"sourceTransform",
			m_source,
			"Firing source transform, used if useSourceTransform is on",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"source",
			m_source.GetTranslation(),
			"Firing source position",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(
			"destination",
			m_destination,
			"Firing target position",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"destinationScale",
			m_destinationScale,
			"Target scale",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"useSourceTransform",
			m_useSourceTransform,
			"Should the effect use the full transform or just a position",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"displaySource",
			m_displaySource,
			"Show/hide effects at the source",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"displayDestination",
			m_displayDestination,
			"Show/hide effects at the destination",
			Be::READWRITE | Be::PERSIST
		)

		EXPOSURE_END()
}
