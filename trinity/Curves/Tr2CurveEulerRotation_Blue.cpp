// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveEulerRotation.h"

BLUE_DEFINE( Tr2CurveEulerRotation );

const Be::ClassInfo* Tr2CurveEulerRotation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveEulerRotation, ":jessica-icon: tree/trirotationcurve.png" )
		MAP_INTERFACE( Tr2CurveEulerRotation )
		MAP_INTERFACE( ITriQuaternionFunction )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"yaw",
			m_yaw,
			"Yaw component curve",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"pitch",
			m_pitch,
			"Pitch component curve",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"roll",
			m_roll,
			"Pitch component curve",
			Be::READ | Be::PERSIST )
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

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddKey",
			AddKey,
			4,
			"Adds a new key to the curve\n"
			":param time: key time\n"
			":param value: key value\n"
			":param interpolation: optional interpolation type for the segment following this key, defaults to trinity.Tr2CurveInterpolation.HERMITE\n"
			":param leftTangent: optional key left (incoming) tangent, defaults to (0, 0, 0)\n"
			":param rightTangent: optional key right (outgoing) tangent, defaults to (0, 0, 0)\n"
			":param tangentType: optional key tangent type, defaults to trinity.Tr2CurveTangentType.AUTO_CLAMP" )

		MAP_METHOD_AND_WRAP(
			"SetExtrapolation",
			SetExtrapolation,
			"Assigns both extrapolationBefore and extrapolationAfter\n"
			":param extrapolation: new extrapolation (trinity.Tr2CurveExtrapolation enum)" )

	EXPOSURE_END()
}