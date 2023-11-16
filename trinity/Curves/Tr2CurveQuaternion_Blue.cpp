////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveQuaternion.h"

extern Be::VarChooser Tr2CurveExtrapolationChooser[];


BLUE_DEFINE( Tr2CurveQuaternion );


const Be::ClassInfo* Tr2CurveQuaternion::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveQuaternion, ":jessica-deprecated: True\n:jessica-icon: tree/trirotationcurve.png" )
		MAP_INTERFACE( Tr2CurveQuaternion )
		MAP_INTERFACE( ITriQuaternionFunction )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE(
			"keys",
			m_keys,
			"Curve control keys",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"extrapolationBefore",
			m_extrapolationBefore,
			"Curve extrapolation type for time before the start of the curve",
			Be::ENUM | Be::READWRITE | Be::PERSIST,
			Tr2CurveExtrapolationChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"extrapolationAfter",
			m_extrapolationAfter,
			"Curve extrapolation type for time after the end of the curve",
			Be::ENUM | Be::READWRITE | Be::PERSIST,
			Tr2CurveExtrapolationChooser )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time"
		)

		MAP_METHOD_AND_WRAP(
			"OnKeysChanged",
			OnKeysChanged,
			"Method to call whenever keys change"
		)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddKey",
			AddKey,
			1,
			"Adds a new key to the curve\n"
			":param time: key time\n"
			":param value: key value\n"
			":param interpolation: optional interpolation type for the segment following this key, defaults to trinity.Tr2CurveInterpolation.LINEAR\n"
		)

		MAP_METHOD_AND_WRAP(
			"SetExtrapolation",
			SetExtrapolation,
			"Assigns both extrapolationBefore and extrapolationAfter\n"
			":param extrapolation: new extrapolation (trinity.Tr2CurveExtrapolation enum)"
		)

		EXPOSURE_END()
}