////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionAnimateValue.h"
#include "Tr2ExpressionTermInfo.h"


BLUE_DEFINE( Tr2ActionAnimateValue );

const Be::ClassInfo* Tr2ActionAnimateValue::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionAnimateValue, "" )
		MAP_INTERFACE( Tr2ActionAnimateValue )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_INTERFACE( ITr2Updateable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "path", m_destination.m_path, "Path to the destination object for shared controllers", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "destination", m_destination.m_object, "Destination object", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "attribute", m_destination.m_attribute, "Destination attribute name", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "value", m_value, "Value expression\n:jessica-widget: expression", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "curve", m_curve, "Curve to control the destination", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"delayBinding",
			m_delayBinding,
			"If binding of the destination path needs to be delayed until the action is executed, rather than \n"
			"binding when the parent controller is linked with the owner. This attribute needs to be turned off most\n"
			"of the time for performance. Valid usages of this are for changing dynamically created\n"
			"subobjects of the parent.",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_PROPERTY_READONLY( "isBindingValid", IsBindingValid, "Is destination binding valid" )
		MAP_PROPERTY_READONLY( "isExpressionValid", IsExpressionValid, "Is value expression valid" )

		MAP_METHOD_AND_WRAP( "GetDestination", GetDestination, "Returns destination object" )
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
