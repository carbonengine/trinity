// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ControllerFloatVariable.h"


namespace
{
Be::VarChooser TypeChooser[] = {
	{ "Float", BeCast( Tr2ControllerFloatVariable::FLOAT ), "Floating point value" },
	{ "Integer", BeCast( Tr2ControllerFloatVariable::INTEGER ), "Integer value" },
	{ "Boolean", BeCast( Tr2ControllerFloatVariable::BOOLEAN ), "Boolean value" },
	{ "Enum", BeCast( Tr2ControllerFloatVariable::ENUM ), "Enumerated value" },
	{ 0 }
};
}

BLUE_DEFINE( Tr2ControllerFloatVariable );

const Be::ClassInfo* Tr2ControllerFloatVariable::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ControllerFloatVariable, "" )
		MAP_INTERFACE( Tr2ControllerFloatVariable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Variable name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "variableType", m_type, "Variable type", Be::READWRITE | Be::PERSIST | Be::ENUM, TypeChooser )
		MAP_ATTRIBUTE( "value", m_value, "Current value", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "defaultValue", m_defaultValue, "Default variable value", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enumValues", m_enumValues, "Coma separated value=name pairs for enum variables", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
