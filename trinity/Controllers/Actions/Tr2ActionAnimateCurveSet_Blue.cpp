////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionAnimateCurveSet.h"
#include "Tr2ExpressionTermInfo.h"


BLUE_DEFINE( Tr2ActionAnimateCurveSet );

const Be::ClassInfo* Tr2ActionAnimateCurveSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionAnimateCurveSet, "" )
		MAP_INTERFACE( Tr2ActionAnimateCurveSet )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_INTERFACE( ITr2Updateable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "value", m_value, "Value expression\n:jessica-widget: expression", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "curveSet", m_curveSet, "Curveset to animate", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "isExpressionValid", IsExpressionValid, "Is value expression valid" )

		MAP_METHOD_AND_WRAP(
			"GetExpressionTermInfo",
			GetExpressionTermInfo,
			"Returns information on addional functions and variables available to the expression" )
		MAP_METHOD_AND_WRAP(
			"IsExpressionValid",
			IsAttrExpressionValid,
			"Checks if the expression is valid\n"
			":param attrName: name of the attribute containing the expression" )
		MAP_METHOD_AND_WRAP(
			"EvaluateExpression",
			EvaluateExpression,
			"Evaluates an expression against this object\n"
			":param expression: expression to evaluate" )
	EXPOSURE_END()
}
