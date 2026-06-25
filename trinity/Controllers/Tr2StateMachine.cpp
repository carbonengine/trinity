// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2StateMachine.h"
#include "Tr2StateMachineState.h"


Tr2StateMachine::Tr2StateMachine( IRoot* lockobj ) :
	PARENTLOCK( m_states ),
	m_controller( nullptr ),
	m_startTime( 0 ),
	m_stateStartTime( 0 )
{
	m_states.SetNotify( this );
	BeOS->RegisterForSimTimeRebase( this );
}

Tr2StateMachine::~Tr2StateMachine()
{
	BeOS->UnregisterForSimTimeRebase( this );
}

void Tr2StateMachine::OnSimClockRebase( Be::Time oldTime, Be::Time newTime )
{
	Be::Time diff = newTime - oldTime;
	m_startTime += diff;
	m_stateStartTime += diff;

	for( auto state = m_states.begin(); state != m_states.end(); ++state )
	{
		( *state )->RebaseSimTime( diff );
	}
}

void Tr2StateMachine::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_states )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( m_controller )
			{
				if( Tr2StateMachineStatePtr state = BlueCastPtr( value ) )
				{
					state->Link( *this );
				}
			}
			break;
		case BELIST_REMOVED:
			if( Tr2StateMachineStatePtr state = BlueCastPtr( value ) )
			{
				if( state == m_currentState )
				{
					state->Stop();
				}
				m_currentState = m_startState;
				m_stateStartTime = BeOS->GetCurrentFrameTime();
				if( m_currentState )
				{
					m_currentState->Start();
				}
				state->Unlink();
			}
			break;
		default:
			break;
		}
	}
}

bool Tr2StateMachine::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_startState ) && m_controller )
	{
		if( m_startState )
		{
			m_startState->Link( *this );
		}
	}
	return true;
}

void Tr2StateMachine::Link( Tr2Controller& controller )
{
	Unlink();

	m_controller = &controller;
	for( auto it = begin( m_states ); it != end( m_states ); ++it )
	{
		( *it )->Link( *this );
	}
}

void Tr2StateMachine::Unlink( UnlinkReason reason )
{
	if( !m_controller )
	{
		return;
	}
	if( reason != UnlinkReason::DELETING )
	{
		Stop();
	}
	for( auto it = begin( m_states ); it != end( m_states ); ++it )
	{
		( *it )->Unlink( reason );
	}
	m_controller = nullptr;
}

void Tr2StateMachine::FollowTransitions( uint64_t variableDirtyMask )
{
	auto next = m_currentState->Update( variableDirtyMask );
	if( !next )
	{
		return;
	}

	std::vector<std::pair<Tr2StateMachineState*, uint32_t>> seen;

	for( uint32_t iteration = 0; next; ++iteration )
	{
		if( iteration > 10 )
		{
			seen.push_back( std::make_pair( m_currentState, 1u ) );

			auto found = std::find_if( begin( seen ), end( seen ), [next]( const auto& x ) {
				return x.first == next;
			} );
			if( found != seen.end() )
			{
				++found->second;
				if( found->second > 20 )
				{
					CCP_LOGERR( "Tr2StateMachine: infinite loop in state machine %s detected", m_name.c_str() );
					return;
				}
			}
			else
			{
				seen.push_back( { next, 1u } );
			}
		}
		m_currentState = next;
		m_currentState->Start();
		m_stateStartTime = BeOS->GetCurrentFrameTime();

		next = m_currentState->Update( 0xffffffffffffffffull );
	}
}

void Tr2StateMachine::Start()
{
	if( m_currentState || !m_controller )
	{
		return;
	}

	m_currentState = m_startState;
	m_startTime = m_stateStartTime = BeOS->GetCurrentFrameTime();
	if( !m_currentState )
	{
		return;
	}
	m_currentState->Start();
	FollowTransitions( 0xffffffffffffffffull );
}

void Tr2StateMachine::Stop()
{
	if( m_currentState )
	{
		m_currentState->Stop();
		m_currentState = nullptr;
	}
	m_startTime = 0;
	m_stateStartTime = 0;
}

void Tr2StateMachine::Update( uint64_t variableDirtyMask )
{
	if( m_currentState )
	{
		FollowTransitions( variableDirtyMask );
	}
}

Tr2Controller* Tr2StateMachine::GetController() const
{
	return m_controller;
}

Tr2StateMachineState* Tr2StateMachine::GetStateByName( const char* name ) const
{
	for( auto it = begin( m_states ); it != end( m_states ); ++it )
	{
		if( ( *it )->GetName() == name )
		{
			return *it;
		}
	}
	return nullptr;
}

float Tr2StateMachine::GetMachineRunTime() const
{
	return m_startTime ? TimeAsFloat( BeOS->GetCurrentFrameTime() - m_startTime ) : 0.f;
}

float Tr2StateMachine::GetStateRunTime() const
{
	return m_stateStartTime ? TimeAsFloat( BeOS->GetCurrentFrameTime() - m_stateStartTime ) : 0.f;
}
