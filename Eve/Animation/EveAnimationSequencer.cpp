#include "StdAfx.h"
#include "EveAnimationSequencer.h"
#include "EveAnimationState.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "../../Tr2GrannyAnimation.h"


EveAnimationStateMachine::EveAnimationStateMachine( IRoot* lockobj ) :
	PARENTLOCK( m_pendingStates ),
	PARENTLOCK( m_states ),
	PARENTLOCK( m_transitions ),
	m_isTransitioning( false ),
	m_update( true ),
	m_autoPlayDefault( false )
{
}

EveAnimationStateMachine::~EveAnimationStateMachine() 
{
}

// --------------------------------------------------------------------------------
// Description:
//   Get the animation state with the name provided(or nullptr if it doesn't exist)
// --------------------------------------------------------------------------------
EveAnimationStatePtr EveAnimationStateMachine::GetAnimationState( const std::string& name, PEveAnimationStateVector& states )
{
	for( size_t i = 0; i < states.size(); i++ )
	{
		if( states[i]->GetName() == name )
		{
			return states[i];
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the owner. Triggers the extra animation.
// --------------------------------------------------------------------------------
void EveAnimationStateMachine::SetOwner( EveSpaceObject2* owner )
{
	if( !owner )
	{
		return;
	}
	
	auto ac = owner->GetAnimationController();
	if( !m_trackMask.empty() )
	{
		ac->AddAnimationLayerWithTrackMask( m_trackMask.c_str(), m_trackMask.c_str() );
	}

	if( m_currentState )
	{
		m_currentState->Start( this, owner, EVE_ANIM_START_INIT );
	}

	if( m_autoPlayDefault && !m_defaultAnimation.empty() )
	{
		if( m_trackMask.empty() )
		{
			ac->PlayAnimationEx( m_defaultAnimation.c_str(), 0, 0, 1 );
		}
		else
		{
			ac->PlayLayerAnimationByName( m_trackMask.c_str(), m_defaultAnimation.c_str(), false, 0, 0, 1, false );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Go to the state specified
// --------------------------------------------------------------------------------
void EveAnimationStateMachine::GoToState( EveSpaceObject2* owner, const std::string& name )
{
	EveAnimationStatePtr state = GetAnimationState( name, m_states );
	if( !state )
	{
		return;
	}

	if( !owner )
	{
		m_currentState = state;
		return;
	}

	if( !m_currentState || m_currentState->GetProgress() == EVE_ANIM_DONE )
	{
		m_currentState = state;
		state->Start( this, owner, EVE_ANIM_START_INIT );
		return;
	}

	EveAnimationStateProgress progress = m_currentState->GetProgress();
	if( progress == EVE_ANIM_RUNNING )
	{
		if( state->GetName() != m_currentState->GetName() )
		{
			m_currentState->Stop( this, owner );
			m_pendingStates.Clear();
			m_pendingStates.Append( state );
		}
		else
		{
			m_pendingStates.Clear();
		}
	}
	else if( progress == EVE_ANIM_FINALIZING )
	{
		if( m_isTransitioning )
		{
			while( m_pendingStates.GetSize() > 1 )
			{
				m_pendingStates.Remove( 1 );
			}
			m_pendingStates.Append( state );
		}
		else
		{
			m_pendingStates.Clear();
			m_pendingStates.Append( state );
		}
	}
}

void EveAnimationStateMachine::ForceState( EveSpaceObject2* owner, const std::string& name )
{
	// Stop the current state if there is any
	if( m_currentState )
	{
		m_currentState->Stop( this, owner );
		m_currentState = nullptr;
	}
	// Remove any pending states
	m_pendingStates.Clear();
		
	GoToState( owner, name );
}

const char* EveAnimationStateMachine::GetEndStateName( )
{
	ssize_t pendingStateSize = m_pendingStates.GetSize();
	if( pendingStateSize == 0 && m_currentState )
	{
		return m_currentState->GetName().c_str();
	}
	else if( pendingStateSize > 0 )
	{
		return m_pendingStates[ pendingStateSize - 1 ]->GetName().c_str();
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Sets state parameters
// --------------------------------------------------------------------------------
void EveAnimationStateMachine::SetStateParameter( const std::string& stateName, const std::string& parameterName, float parameterValue )
{
	EveAnimationStatePtr state = GetAnimationState( stateName, m_states );
	if( !state )
	{
		return;
	}
	state->SetParameter( parameterName, parameterValue );
}

// --------------------------------------------------------------------------------
// Description:
//   Check if the current state is complete and swap for a new state if one is
//   available.
// --------------------------------------------------------------------------------
bool EveAnimationStateMachine::CheckCompletionAndChangeStates( EveSpaceObject2* owner )
{
	ssize_t pendingCount = m_pendingStates.GetSize();
	if( m_currentState->GetProgress() != EVE_ANIM_DONE || !pendingCount )
	{
		return false;
	}
	
	EveAnimationStatePtr lastState = m_pendingStates[pendingCount - 1];
	EveAnimationStatePtr nextState = lastState;
	const char* stateName;
	// We have a pending state
	if( pendingCount == 1 )
	{
		// Current state is done and we were not transitioning and the newly finished state has a transition to the pending state
		if( !m_isTransitioning && (stateName=m_currentState->GetTransition( lastState->GetName() )) )
		{
			// So we start transitioning into the pending state(the transition will be the new current state)
			// We don't clear the pending list because the pending state is the next one we play after the transition(see next else if block)
			nextState = GetAnimationState( stateName, m_transitions );
			m_isTransitioning = true;
			nextState->Start( this, owner, EVE_ANIM_START_TRANSITION );
		}
		// We only had one pending state and we're already transitioning into it.
		else if( m_isTransitioning )
		{
			// Clear the single pending state from the list, that will be the result of out newly finished transition
			m_pendingStates.Clear();
			m_isTransitioning = false;
			nextState->Start( this, owner );
		}
		// We have a pending state but no transition into it, which can be fine I guess
		else
		{
			// Just start the pending state
			m_pendingStates.Clear();
			m_isTransitioning = false;
			nextState->Start( this, owner, EVE_ANIM_START_INIT );
		}
	}
	// We have more than one(max of 2 states allowed, see GoToState) and we're already transitioning into the first pending state
	else if( m_isTransitioning )
	{
		if( ( stateName = m_pendingStates[0]->GetTransition( lastState->GetName() ) ) )
		{
			// We're just finished transitioning into the first pending state. That state has a transition
			// into the last pending state so we start transitioning into that immediately.
			nextState = GetAnimationState( stateName, m_transitions );
			// The final destination state is the only state left on the pending list
			m_pendingStates.Clear();
			m_pendingStates.Append( lastState );
			nextState->Start( this, owner, EVE_ANIM_START_TRANSITION );
		}
		else
		{
			// Transition is done, we don't have a transition into the next state so we just play it directly
			m_pendingStates.Clear();
			m_isTransitioning = false;
			nextState->Start( this, owner, EVE_ANIM_START_INIT );
		}
	}
	//else{} only way get more than 1 pending is if we're transitioning, see GoToState
	m_currentState = nextState;

	return true;
}

void EveAnimationStateMachine::Update( EveSpaceObject2* owner, Be::Time time )
{
	if( !m_currentState || !m_update || !owner )
	{
		return;
	}

	do 
	{
		m_currentState->Update( time, owner );
	} while( CheckCompletionAndChangeStates( owner ) );
}

void EveAnimationStateMachine::Clear()
{
	m_currentState = nullptr;
	m_pendingStates.Clear();
}



EveAnimationSequencer::EveAnimationSequencer( IRoot* lockobj ) :
	PARENTLOCK( m_stateMachines )
{
	m_stateMachines.SetNotify( this );
}


EveAnimationSequencer::~EveAnimationSequencer()
{
}


void EveAnimationSequencer::SetOwner( EveSpaceObject2* owner )
{
	if( m_owner == owner )
	{
		return;
	}

	m_owner = owner;
	for( auto it = m_stateMachines.begin(); it != m_stateMachines.end(); it++ )
	{
		(*it)->SetOwner( owner );
	}
}


void EveAnimationSequencer::Update( Be::Time time )
{
	for( auto it = m_stateMachines.begin(); it != m_stateMachines.end(); it++ )
	{
		(*it)->Update( m_owner, time );
	}
}


void EveAnimationSequencer::GoToState( const std::string& name )
{
	if( !m_owner )
	{
		return;
	}

	IRoot* owner = m_owner;
	owner->Lock();
	for( auto it = m_stateMachines.begin(); it != m_stateMachines.end(); it++ )
	{
		(*it)->GoToState( m_owner, name );
	}
	owner->Unlock();
}

void EveAnimationSequencer::ForceState( const std::string& name )
{
	if( !m_owner )
	{
		return;
	}

	IRoot* owner = m_owner;
	owner->Lock();

	for( auto it = m_stateMachines.begin(); it != m_stateMachines.end(); it++ )
	{
		(*it)->ForceState( m_owner, name );
	}
	owner->Unlock();
}

void EveAnimationSequencer::SetStateParameter( const std::string& stateName, const std::string& parameterName, float parameterValue )
{
	for( auto it = m_stateMachines.begin(); it != m_stateMachines.end(); it++ )
	{
		(*it)->SetStateParameter( stateName, parameterName, parameterValue);		
	}
	
}

void EveAnimationSequencer::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const IList* theList
	)
{
	if( theList != &m_stateMachines || !value )
	{
		return;
	}

	if( !m_owner || ( event & BELIST_EVENTMASK ) != BELIST_INSERTED )
	{
		return;
	}

	EveAnimationStateMachinePtr sm;
	if( value->QueryInterface( BlueInterfaceIID<EveAnimationStateMachine>(), (void**)&sm, BEQI_SILENT ) )
	{
		sm->SetOwner( m_owner );
	}
}