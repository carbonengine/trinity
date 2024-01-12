#include "StdAfx.h"
#include "Tr2CurveVector3Expression.h"
#include "Tr2ExpressionTermInfo.h"

BLUE_DEFINE( Tr2CurveVector3Expression );

const Be::ClassInfo* Tr2CurveVector3Expression::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveVector3Expression, ":jessica-icon: tree/CurveExpression.png" )
		MAP_INTERFACE( Tr2CurveVector3Expression )
		MAP_INTERFACE( ITriColorFunction )
		MAP_INTERFACE( ITriVectorFunction )
	{
			Be::InterfaceEntry entry = { &GetITriFunctionIID(), BLUE_INTERFACEOFFSET( ITriColorFunction ) };
			s_interfaces.push_back( entry );
		}
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"expressionX",
			m_expressions[0],
			"Curve expression for x component\n:jessica-widget: expression",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expressionX",
			GetExpressionX,
			SetExpressionX,
			"Curve expression for x component\n:jessica-widget: expression" )
		MAP_ATTRIBUTE(
			"expressionY",
			m_expressions[1],
			"Curve expression for y component\n:jessica-widget: expression",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expressionY",
			GetExpressionY,
			SetExpressionY,
			"Curve expression for y component" )
		MAP_ATTRIBUTE(
			"expressionZ",
			m_expressions[2],
			"Curve expression for z component",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"expressionZ",
			GetExpressionZ,
			SetExpressionZ,
			"Curve expression for z component" )

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
			":param time: input time"
		)

		MAP_METHOD_AND_WRAP(
			"ResetRandomConstant",
			ResetRandomConstant,
			"Resets curve random constant to a new random value\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/refreshrandom.png"
		)

		MAP_METHOD_AND_WRAP(
			"GetExpressionTermInfo",
			GetExpressionTermInfo,
			"Returns information on addional functions and variables available to the expression"
		)
		MAP_METHOD_AND_WRAP(
			"EvaluateExpression",
			EvaluateExpression,
			"Evaluates an expression against this object\n"
			":param expression: expression to evaluate"
		)

	EXPOSURE_END()
}
