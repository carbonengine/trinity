////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2StateMachineTransition.h"
#include "Tr2StateMachineState.h"
#include "Tr2ExpressionTermInfo.h"


BLUE_DEFINE( Tr2StateMachineTransition );


const Be::ClassInfo* Tr2StateMachineTransition::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2StateMachineTransition, "" )
		MAP_INTERFACE( Tr2StateMachineTransition )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_destinationName, "Destination state name", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "condition", m_condition, "Condition expression\n:jessica-widget: expression", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_PROPERTY_READONLY( "isConditionValid", IsConditionValid, "" )

		MAP_METHOD_AND_WRAP( "GetState", GetSource, "" )

		MAP_METHOD_AND_WRAP( 
			"GetExpressionTermInfo", 
			GetExpressionTermInfo, 
			"Returns information on addional functions and variables available to the expression" )
		MAP_METHOD_AND_WRAP( 
			"IsExpressionValid", 
			IsExpressionValid,
			"Checks if the expression is valid\n"
			":param attrName: name of the attribute containing the expression" )
		MAP_METHOD_AND_WRAP(
			"EvaluateExpression",
			EvaluateExpression,
			"Evaluates an expression against this object\n"
			":param expression: expression to evaluate" )
	EXPOSURE_END()
}
