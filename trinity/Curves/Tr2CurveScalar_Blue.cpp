////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveScalar.h"


BLUE_DEFINE( Tr2CurveScalar );

Be::VarChooser Tr2CurveInterpolationChooser[] =
{
	{
		"CONSTANT",
		BeCast( Tr2CurveInterpolation::CONSTANT ),
		"Performs a constant interpolation"
	},
	{
		"LINEAR",
		BeCast( Tr2CurveInterpolation::LINEAR ),
		"Performs a linear interpolation"
	},
	{
		"HERMITE",
		BeCast( Tr2CurveInterpolation::HERMITE ),
		"Performs a hermite interpolation"
	},
	{ 0 }
};

Be::VarChooser Tr2CurveTangentTypeChooser[] =
{
	{
		"AUTO_CLAMP",
		BeCast( Tr2CurveTangentType::AUTO_CLAMP ),
		"Automatically adjust tangents clamping overshoots"
	},
	{
		"AUTO",
		BeCast( Tr2CurveTangentType::AUTO ),
		"Automatically adjust tangents"
	},
	{
		"FREE_JOINED",
		BeCast( Tr2CurveTangentType::FREE_JOINED ),
		"Manually adjusted unified tangents"
	},
	{
		"FREE_SPLIT",
		BeCast( Tr2CurveTangentType::FREE_SPLIT ),
		"Manually adjusted broken tangents"
	},
	{ 0 }
};

Be::VarChooser Tr2CurveExtrapolationChooser[] =
{
	{
		"CLAMP",
		BeCast( Tr2CurveExtrapolation::CLAMP ),
		"Use start/end values"
	},
	{
		"CYCLE",
		BeCast( Tr2CurveExtrapolation::CYCLE ),
		"Cycle the curve"
	},
	{
		"MIRROR",
		BeCast( Tr2CurveExtrapolation::MIRROR ),
		"Mirror the curve"
	},
	{
		"LINEAR",
		BeCast( Tr2CurveExtrapolation::LINEAR ),
		"Linear exprapolation based on first/last key tangent"
	},
	{ 0 }
};


BLUE_REGISTER_ENUM_EX( "Tr2CurveExtrapolation", Tr2CurveExtrapolation::Type, Tr2CurveExtrapolationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
BLUE_REGISTER_ENUM_EX( "Tr2CurveInterpolation", Tr2CurveInterpolation::Type, Tr2CurveInterpolationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
BLUE_REGISTER_ENUM_EX( "Tr2CurveTangentType", Tr2CurveTangentType::Type, Tr2CurveTangentTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* Tr2CurveScalar::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveScalar, ":jessica-icon: tree/triscalarcurve.png" )
		MAP_INTERFACE( Tr2CurveScalar )
		MAP_INTERFACE( ITriScalarFunction )
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

		MAP_ATTRIBUTE( 
			"timeOffset", 
			m_timeOffset, 
			"Curve internal time offset", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"timeScale", 
			m_timeScale, 
			"Curve internal time scale", 
			Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time"
		)

		MAP_METHOD_AND_WRAP(
			"GetTangentAt",
			GetTangent,
			"Returns the tangent to the curve at specified time\n"
			":param time: input time"
		)

		MAP_METHOD_AND_WRAP(
			"OnKeysChanged",
			OnKeysChanged,
			"Method to call whenever keys change"
		)

		MAP_METHOD_AND_WRAP(
			"GetTimeOffset",
			GetTimeOffset,
			"Returns internal time offset in seconds"
		)

		MAP_METHOD_AND_WRAP(
			"SetTimeOffset",
			SetTimeOffset,
			"Set new time offset in seconds\n"
			":param timeOffset: new time offset"
		)

		MAP_METHOD_AND_WRAP(
			"GetTimeScale",
			GetTimeScale,
			"Returns internal time scaling factor"
		)

		MAP_METHOD_AND_WRAP(
			"SetTimeScale",
			SetTimeScale,
			"Set new time scaling factor\n"
			":param timeScaling: new time scaling factor"
		)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddKey",
			AddKey,
			4,
			"Adds a new key to the curve\n"
			":param time: key time\n"
			":param value: key value\n"
			":param interpolation: optional interpolation type for the segment following this key, defaults to trinity.Tr2CurveInterpolation.HERMITE\n"
			":param leftTangent: optional key left (incoming) tangent, defaults to 0\n"
			":param rightTangent: optional key right (outgoing) tangent, defaults to 0\n"
			":param tangentType: optional key tangent type, defaults to trinity.Tr2CurveTangentType.AUTO_CLAMP"
		)

		MAP_METHOD_AND_WRAP(
			"SetExtrapolation",
			SetExtrapolation,
			"Assigns both extrapolationBefore and extrapolationAfter\n"
			":param extrapolation: new extrapolation (trinity.Tr2CurveExtrapolation enum)"
		)

	EXPOSURE_END()
}