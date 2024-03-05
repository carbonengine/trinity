////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2StateMachineState.h"
#include "Finalizers/ITr2StateMachineStateFinalizer.h"

BLUE_DEFINE_INTERFACE( ITr2StateMachineStateFinalizer );

BLUE_DEFINE( Tr2StateMachineState );

const Be::ClassInfo* Tr2StateMachineState::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2StateMachineState, "" )
		MAP_INTERFACE( Tr2StateMachineState )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "actions", m_actions, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transitions", m_transitions, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "finalizer", m_finalizer, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_METHOD_AND_WRAP( "GetStateMachine", GetStateMachinePtr, "" )
	EXPOSURE_END()
}
