// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveRandomAxisRotation.h"

BLUE_DEFINE( Tr2CurveRandomAxisRotation );

const Be::ClassInfo* Tr2CurveRandomAxisRotation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveRandomAxisRotation, ":jessica-icon: tree/trirotationcurve.png" )
		MAP_INTERFACE( Tr2CurveRandomAxisRotation )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriQuaternionFunction )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"period",
			m_period,
			"Full rotation period in seconds",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"seed",
			m_seed,
			"Random seed number",
			Be::PERSISTONLY )

		MAP_PROPERTY(
			"seed",
			GetSeed,
			SetSeed,
			"Random seed number" )

		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time" )

	EXPOSURE_END()
}