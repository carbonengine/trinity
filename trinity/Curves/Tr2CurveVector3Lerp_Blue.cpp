// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveVector3Lerp.h"


Be::VarChooser Tr2CurveVector3LerpKeyInterpolationChooser[] = {
	{ "LINEAR",
	  BeCast( Tr2CurveVector3LerpKeyInterpolation::LINEAR ),
	  "Performs a linear interpolation" },
	{ "HERMITE",
	  BeCast( Tr2CurveVector3LerpKeyInterpolation::HERMITE ),
	  "Performs a hermite interpolation" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "Tr2CurveVector3LerpKeyInterpolation", Tr2CurveVector3LerpKeyInterpolation::Type, Tr2CurveVector3LerpKeyInterpolationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( Tr2CurveVector3Lerp );
const Be::ClassInfo* Tr2CurveVector3Lerp::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveVector3Lerp, ":jessica-icon: tree/trivectorcurve.png" )
		MAP_INTERFACE( Tr2CurveVector3Lerp )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriVectorFunction )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"initialValue",
			m_initialValue,
			"Initial value of the curve",
			Be::READWRITE )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"startInterpolation",
			m_startInterpolation,
			"The interpolation from initial value to first key",
			Be::ENUM | Be::READWRITE,
			Tr2CurveVector3LerpKeyInterpolationChooser )

		MAP_ATTRIBUTE(
			"curve",
			m_curve,
			"Curve that takes over after the initial value",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"curveStartTime",
			m_curveStartTime,
			"The time when the curve starts",
			Be::READWRITE | Be::PERSIST )

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
