////////////////////////////////////////////////////////////
//
//    Created:   October 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ControllerEventHandler.h"

BLUE_DEFINE( Tr2ControllerEventHandler );

const Be::ClassInfo* Tr2ControllerEventHandler::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ControllerEventHandler, "" )
		MAP_INTERFACE( Tr2ControllerEventHandler )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "Event name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "actions", m_actions, "Actions to execute", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
