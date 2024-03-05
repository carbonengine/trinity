////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2ActionBindRTPC.h"
#include "Tr2ExpressionTermInfo.h"


BLUE_DEFINE( Tr2ActionBindRTPC );

const Be::ClassInfo* Tr2ActionBindRTPC::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionBindRTPC, "" )
		MAP_INTERFACE( Tr2ActionBindRTPC )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_INTERFACE( ITr2Updateable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "value", m_value, "Value expression\n:jessica-widget: expression", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "emitter", m_emitterName, "The name of the audio emitter to send the RTPC to.\n:jessica-widget: observer-name", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "rtpcName", m_rtpcName, "The name of the RTPC to bind to in Wwise.", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "curve", m_curve, "A curve to get running time from.", Be::READWRITE | Be::PERSIST )

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
		MAP_METHOD_AND_WRAP(
			"StartWithController",
			StartWithController,
			"Starts the action manually without changing state machine state.\n"
			":type controller: Tr2Controller\n"
			":param controller: The controller sent to the start method of the action.\n" )
		MAP_METHOD_AND_WRAP(
			"StopWithController",
			StopWithController,
			"Stops the action manually without changing state machine state.\n"
			":type controller: Tr2Controller\n"
			":param controller: The controller sent to the start method of the action.\n" )
	EXPOSURE_END()
}
