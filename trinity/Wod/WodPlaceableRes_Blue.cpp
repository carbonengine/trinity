// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "WodPlaceableRes.h"

BLUE_DEFINE( WodPlaceableRes );

const Be::ClassInfo* WodPlaceableRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( WodPlaceableRes, "" )
		MAP_INTERFACE( WodPlaceableRes )

		MAP_ATTRIBUTE( "visualModel", m_visualModel, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "farFadeDistance", m_farFadeDistance, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "nearFadeDistance", m_nearFadeDistance, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"isShadowCaster",
			m_isShadowCaster,
			"Is this placeable res a shadow caster?",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
