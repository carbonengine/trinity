// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ScalarExprKeyCurve.h"

BLUE_DEFINE( Tr2ScalarExprKeyCurve );
BLUE_DEFINE( Tr2ScalarExprKey );

Be::VarChooser ScalarInterpolationChooser[] = {
	{ "CONSTANT",
	  BeCast( CONSTANT ),
	  "Performs a constant interpolation" },
	{ "LINEAR",
	  BeCast( LINEAR ),
	  "Performs a linear interpolation" },
	{ "HERMITE",
	  BeCast( HERMITE ),
	  "Performs a Hermite spline interpolation" },
	{ 0 }
};

const Be::ClassInfo* Tr2ScalarExprKey::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ScalarExprKey, "" )
		MAP_INTERFACE( Tr2ScalarExprKey )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "time", m_time, "Key time", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "value", m_value, "Key value", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "left", m_leftTangent, "Left tangent value", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "right", m_rightTangent, "Right tangent value", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE( "timeExpression", m_timeExpression, "Math expression for key time", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "valueExpression", m_valueExpression, "Math expression for key value", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "leftTangentExpression", m_leftTangentExpression, "Math expression for key left tangent value", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "rightTangentExpression", m_rightTangentExpression, "Math expression for key right tangent value", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "input1", m_inputVar1, "Artbitrary input variable for key expressions", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "input2", m_inputVar2, "Artbitrary input variable for key expressions", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "input3", m_inputVar3, "Artbitrary input variable for key expressions", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "input4", m_inputVar4, "Artbitrary input variable for key expressions", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "randomConstant", m_randomConstant, "Random constant for key expressions", Be::READ )
		MAP_ATTRIBUTE( "randomMin", m_randomMin, "Min range for randomConstant", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "randomMax", m_randomMax, "Max range for randomConstant", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE( "prevKeyTime", m_prevKeyTime, "Time of the previous key", Be::READ )
		MAP_ATTRIBUTE( "prevKeyValue", m_prevKeyValue, "Value of the previous key", Be::READ )


		MAP_ATTRIBUTE_WITH_CHOOSER( "interpolation", m_interpolation, "Curve interpolation at key", Be::READWRITE | Be::PERSIST | Be::ENUM, ScalarInterpolationChooser )

		MAP_METHOD_AND_WRAP( "RegenRandomConstant", RegenRandomConstant, "Regenerate the randomConstant variable for expressions" )
	EXPOSURE_END()
}

const Be::ClassInfo* Tr2ScalarExprKeyCurve::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ScalarExprKeyCurve, ":jessica-deprecated:" )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "length", Length, "" )
		MAP_ATTRIBUTE( "cycle", m_cycle, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reversed", m_reversed, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "timeOffset", m_timeOffset, "An internal offset to the curve's timing", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "timeScale", m_timeScale, "An internal scaling to the curve's timing", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "currentValue", m_currentValue, "", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER( "interpolation", m_interpolation, "", Be::READWRITE | Be::PERSIST | Be::ENUM, ScalarInterpolationChooser )
		MAP_ATTRIBUTE( "keys", m_keys, "These are the keys of the curve", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddKey",
			AddKey,
			3,
			"Adds a new key to the curve\n"
			":param time: key time\n"
			":param value: key value\n"
			":param leftTangent: key left tangent value\n"
			":param rightTangent: key right tangent value\n"
			":param interpolation: key interpolation type" );
		MAP_METHOD_AND_WRAP(
			"RemoveKey",
			RemoveKey,
			"Removes a key\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP( "GetKeyCount", GetKeyCount, "Returns number of keys" );
		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValueAt,
			"Returns curve value at a given time\n"
			":param time: input time" );
		MAP_METHOD_AND_WRAP(
			"GetKeyValue",
			GetKeyValue,
			"Returns key value\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP(
			"GetKeyTime",
			GetKeyTime,
			"Returns key time\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP(
			"GetKeyLeftTangent",
			GetKeyLeftTangent,
			"Returns key left tangent value\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP(
			"GetKeyRightTangent",
			GetKeyRightTangent,
			"Returns key left tangent value\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP(
			"GetKeyInterpolation",
			GetKeyInterpolation,
			"Returns interpolation type for the key\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP(
			"SetKeyValue",
			SetKeyValue,
			"Changes key value\n"
			":param idx: key index\n"
			":param value: new key value" );
		MAP_METHOD_AND_WRAP(
			"SetKeyTime",
			SetKeyTime,
			"Changes key time.\n"
			"You need to call Sort() afterwards, to make sure the keys are in the correct order.\n"
			":param idx: key index\n"
			":param time: new key time" );
		MAP_METHOD_AND_WRAP(
			"SetKeyLeftTangent",
			SetKeyLeftTangent,
			"Changes key left tangent value"
			":param idx: key index\n"
			":param value: new key left tangent value" );
		MAP_METHOD_AND_WRAP(
			"SetKeyRightTangent",
			SetKeyRightTangent,
			"Changes key right tangent value"
			":param idx: key index\n"
			":param value: new key left tangent value" );
		MAP_METHOD_AND_WRAP(
			"SetKeyInterpolation",
			SetKeyInterpolation,
			"Changes key interpolation type\n"
			":param idx: key index\n"
			":param interpolation: new interpolation type" );
		MAP_METHOD_AND_WRAP( "Sort", Sort, "Re-evaluate key expressions" );
	EXPOSURE_END()
}
