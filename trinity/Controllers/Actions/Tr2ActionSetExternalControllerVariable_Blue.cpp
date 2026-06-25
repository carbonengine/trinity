// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionSetExternalControllerVariable.h"


BLUE_DEFINE( Tr2ActionSetExternalControllerVariable );
BLUE_DEFINE_INTERFACE( ITr2ControllerOwner );

const Be::ClassInfo* Tr2ActionSetExternalControllerVariable::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionSetExternalControllerVariable, "" )
		MAP_INTERFACE( Tr2ActionSetExternalControllerVariable )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "destinationOwner", m_destinationOwner, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "destination", m_destination, "", Be::READ )
		MAP_ATTRIBUTE( "variable", m_variable, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "value", m_value, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sourceVariable", m_sourceVariable, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startControllers", m_startControllers, "", Be::READWRITE | Be::PERSIST )

		MAP_PROPERTY_READONLY( "destinationIsValid", IsDestinationValid, "" )


	EXPOSURE_END()
}
