// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveStarfield.h"

BLUE_DEFINE( EveStarfield );

const Be::ClassInfo* EveStarfield::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveStarfield, "" )
		MAP_INTERFACE( EveStarfield )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"display",
			m_display,
			"na",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"numStars",
			m_starCount,
			"Total number of stars.",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"seed",
			m_seed,
			"A seed that controls star generation.",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"maxDist",
			m_maxDistance,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"minDist",
			m_minDistance,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"minFlashRate",
			m_minFlashRate,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"maxFlashRate",
			m_maxFlashRate,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"minFlashIntensity",
			m_minFlashIntensity,
			"Determines how bright a star flashes. Should be a number between 0 and 1.",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"The effect used to draw individual stars.",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}