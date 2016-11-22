#include "StdAfx.h"
#include "EveAnimationSequencer.h"
#include "EveAnimationState.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"

BLUE_DEFINE( EveAnimationStateMachine );

const Be::VarChooser EveAnimationProgressChooser[] =
{
	// Name						Value								Docstring
	{ "EVE_ANIM_DONE",			BeCast( EVE_ANIM_DONE ),			"Animation state done" }, 
	{ "EVE_ANIM_INACTIVE",		BeCast( EVE_ANIM_INACTIVE ),		"Animation state inactive(no actions taken yet)" }, 
	{ "EVE_ANIM_FINALIZING",	BeCast( EVE_ANIM_FINALIZING ),		"Animation is finishing and waiting to be replaced" }, 
	{ "EVE_ANIM_RUNNING",		BeCast( EVE_ANIM_RUNNING ),			"Animation state active, i.e. running it's main animations" }, 
	{0}
};

const Be::ClassInfo* EveAnimationStateMachine::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveAnimationStateMachine, "" )
        MAP_INTERFACE( EveAnimationStateMachine )
        MAP_INTERFACE( IRoot )
		
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "update", m_update, "", Be::READWRITE )
		MAP_ATTRIBUTE( "autoPlayDefault", m_autoPlayDefault, "Should default animation be played when the object is loaded.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentState", m_currentState, "", Be::READ )
		MAP_ATTRIBUTE( "pendingStates", m_pendingStates, "", Be::READ )
		MAP_ATTRIBUTE( "states", m_states, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transitions", m_transitions, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Clear", Clear, "" )
		MAP_METHOD_AND_WRAP( "GoToState", GoToState, "" )
		MAP_METHOD_AND_WRAP( "GetEndStateName", GetEndStateName, "Returns the name of the last of the pending states" )
		
		MAP_ATTRIBUTE( "defaultState", m_defaultState, "State that should be used for previews etc.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultAnimation", m_defaultAnimation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trackMask", m_trackMask, "", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}


BLUE_DEFINE( EveAnimationSequencer );

const Be::ClassInfo* EveAnimationSequencer::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveAnimationSequencer, "" )
        MAP_INTERFACE( EveAnimationSequencer )
        MAP_INTERFACE( IListNotify )
		
		MAP_ATTRIBUTE( "owner", m_owner, "", Be::READ )
		MAP_ATTRIBUTE( "stateMachines", m_stateMachines, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "GoToState", GoToState, "" )
		MAP_METHOD_AND_WRAP( "ForceState", ForceState, "Forces the statemachines to go into a specific state" )
		MAP_METHOD_AND_WRAP( "SetStateParameter", SetStateParameter, "" )
    EXPOSURE_END()
}