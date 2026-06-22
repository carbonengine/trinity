// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionPython.h"


BLUE_DEFINE( Tr2ActionPython );

const Be::ClassInfo* Tr2ActionPython::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionPython, "" )
		MAP_INTERFACE( Tr2ActionPython )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_INTERFACE( ITr2Updateable )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ICustomPersist )

		MAP_ATTRIBUTE( "module", m_module, "Full import path to the module containing the action implementation class", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "className", m_className, "Name of the action implementation class", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_METHOD_AND_WRAP( "GetInstance", GetInstance, "Returns Python implementation object instance" )
		MAP_ATTRIBUTE_AS_CUSTOM_BINARY_BLOCK( "state" )

	EXPOSURE_END()
}
