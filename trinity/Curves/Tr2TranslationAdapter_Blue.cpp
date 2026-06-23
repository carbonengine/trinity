// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2TranslationAdapter.h"


BLUE_DEFINE( Tr2TranslationAdapter );

const Be::ClassInfo* Tr2TranslationAdapter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TranslationAdapter, ":jessica-icon: tree/trivectorcurve.png" )
		MAP_INTERFACE( Tr2TranslationAdapter )
		MAP_INTERFACE( ITriVectorFunction )

		MAP_ATTRIBUTE(
			"value",
			m_value,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"curve",
			m_curve,
			"Attached curve",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"rotationOffset",
			m_rotationOffset,
			"use to calculate additional modelTranslation",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"RandomizeStart",
			RandomizeStart,
			1,
			"Shifts curve starting time by a random number in a specified range (-rng, +rng)\n"
			":param rng: radius of the random range in seconds (default is 60 sec)" )

		MAP_METHOD_AND_WRAP(
			"ScaleTime",
			ScaleTime,
			"Scales curve time\n"
			":param scale: new time scaling factor" )

		MAP_METHOD_AND_WRAP(
			"ResetStart",
			ResetStart,
			"Reset start time" )

	EXPOSURE_END()
}