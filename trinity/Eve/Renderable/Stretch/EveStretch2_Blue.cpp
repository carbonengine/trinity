// Copyright © 2017 CCP ehf.

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
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"Effect to use for the stretch",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"quadCount",
			m_quadCount,
			"Number of quads to render for the effect",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"start",
			m_start,
			"Curve set to play when firing effect starts",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"loop",
			m_loop,
			"Curve set that is played when the effect is active",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"end",
			m_end,
			"Curve set to play when the effect stops",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sourceLight",
			m_sourceLight,
			"Point light at the effect source",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"destinationLight",
			m_destinationLight,
			"Point light at the effect destination",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sourceEmitter",
			m_sourceEmitter,
			"GPU particle emitter at the source",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"destinationEmitter",
			m_destinationEmitter,
			"GPU particle emitter at the destination",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sourceObserver",
			m_sourceObserver,
			"Observer at the source position",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"destinationObserver",
			m_destinationObserver,
			"Observer at the destination position",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"boundingRadius",
			m_boundingRadius,
			"Radius for the bounding cylinder used for frustum culling",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
