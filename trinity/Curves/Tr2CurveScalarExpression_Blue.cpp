// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveScalarExpression.h"
#include "Tr2ExpressionTermInfo.h"

BLUE_DEFINE( Tr2CurveScalarExpression );

const Be::ClassInfo* Tr2CurveScalarExpression::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveScalarExpression, ":jessica-icon: tree/CurveExpression.png" )
		MAP_INTERFACE( Tr2CurveScalarExpression )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriScalarFunction )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"expression",
			m_expression,
			"Curve expression\n:jessica-widget: expression",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expression",
			GetExpression,
			SetExpression,
			"Curve expression\n:jessica-widget: expression" )
		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )
		MAP_ATTRIBUTE(
			"inputs",
			m_inputs,
			"Scalar curve inputs",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"input1",
			m_arguments.m_input1,
			"Input variable",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"input2",
			m_arguments.m_input2,
			"Input variable",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"input3",
			m_arguments.m_input3,
			"Input variable",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"input4",
			m_arguments.m_input4,
			"Input variable",
			Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time" )

		MAP_METHOD_AND_WRAP(
			"ResetRandomConstant",
			ResetRandomConstant,
			"Resets curve random constant to a new random value\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/refreshrandom.png" )

		MAP_METHOD_AND_WRAP(
			"GetExpressionTermInfo",
			GetExpressionTermInfo,
			"Returns information on addional functions and variables available to the expression" )
		MAP_METHOD_AND_WRAP(
			"EvaluateExpression",
			EvaluateExpression,
			"Evaluates an expression against this object\n"
			":param expression: expression to evaluate" )

	EXPOSURE_END()
}
