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
EveAnimationStatePtr EveAnimationStateMachine::GetAnimationState( const std::string& name, const PEveAnimationStateVector& states )
{
	for( auto it = states.begin(); it != states.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			return *it;
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
			m_currentState->Stop( owner );
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
	if( pendingCount == 1 )
	{
		if( !m_isTransitioning && (stateName=m_currentState->GetTransition( lastState->GetName() )) )
		{
			nextState = GetAnimationState( stateName, m_transitions );
			m_isTransitioning = true;
			nextState->Start( this, owner, EVE_ANIM_START_TRANSITION );
		}
		else if( m_isTransitioning )
		{
			m_pendingStates.Clear();
			m_isTransitioning = false;
			nextState->Start( this, owner );
		}
		else
		{
			m_pendingStates.Clear();
			m_isTransitioning = false;
			nextState->Start( this, owner, EVE_ANIM_START_INIT );
		}
	}
	else if( m_isTransitioning )
	{
		if( ( stateName = m_pendingStates[0]->GetTransition( lastState->GetName() ) ) )
		{
			// We're just finished transitioning into the first pending state. That state has a transition
			// into the last pending state so we start transitioning into that immediately.
			nextState = GetAnimationState( stateName, m_transitions );
			m_pendingStates.Clear();
			m_pendingStates.Append( lastState );
			nextState->Start( this, owner, EVE_ANIM_START_TRANSITION );
		}
		else
		{
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
	for( auto it = m_stateMachines.begin(); it != m_stateMachines.end(); it++ )
	{
		(*it)->GoToState( m_owner, name );
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