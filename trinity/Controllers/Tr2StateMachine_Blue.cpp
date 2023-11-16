////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2StateMachine.h"


BLUE_DEFINE( Tr2StateMachine );

const Be::ClassInfo* Tr2StateMachine::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2StateMachine, "" )
		MAP_INTERFACE( Tr2StateMachine )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "states", m_states, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "startState", m_startState, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "currentState", m_currentState, "", Be::READ )
	EXPOSURE_END()
}
