// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2StateMachineState.h"
#include "Tr2StateMachine.h"
#include "Tr2Controller.h"
#include "Tr2StateMachineTransition.h"
#include "Actions/ITr2ControllerAction.h"
#include "Finalizers/ITr2StateMachineStateFinalizer.h"
#include "ContinueOnMainThread.h"


Tr2StateMachineState::Tr2StateMachineState( IRoot* lockobj ) :
	PARENTLOCK( m_actions ),
	PARENTLOCK( m_transitions ),
	m_stateMachine( nullptr ),
	m_transitionVariableMask( 0 ),
	m_isActive( false ),
	m_isFinalizing( false ),
	m_hasBeenVetoed( false )
{
	m_actions.SetNotify( this );
	m_transitions.SetNotify( this );
}

bool Tr2StateMachineState::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_finalizer ) )
	{
		if( m_finalizer && m_stateMachine )
		{
			m_finalizer->Link( *m_stateMachine->GetController() );
		}
	}
	return true;
}

void Tr2StateMachineState::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_actions )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( m_stateMachine )
			{
				if( ITr2ControllerActionPtr action = BlueCastPtr( value ) )
				{
					action->Link( *m_stateMachine->GetController() );
					if( m_isActive )
					{
						action->Start( *m_stateMachine->GetController() );
					}
				}
			}
			break;
		case BELIST_REMOVED:
			if( ITr2ControllerActionPtr action = BlueCastPtr( value ) )
			{
				if( m_stateMachine )
				{
					if( m_isActive )
					{
						action->Stop( *m_stateMachine->GetController() );
					}
				}
				action->Unlink();
			}
			break;
		default:
			break;
		}
	}
	else if( list == &m_transitions )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( m_stateMachine )
			{
				if( Tr2StateMachineTransitionPtr transition = BlueCastPtr( value ) )
				{
					transition->Link( *this );
					UpdateVariableMask();
				}
			}
			break;
		case BELIST_REMOVED:
			if( Tr2StateMachineTransitionPtr transition = BlueCastPtr( value ) )
			{
				transition->Unlink();
				UpdateVariableMask();
			}
			break;
		default:
			break;
		}
	}
}

void Tr2StateMachineState::Link( const Tr2StateMachine& stateMachine )
{
	Unlink();

	m_stateMachine = &stateMachine;
	m_transitionVariableMask = 0;
	bool hasMask = true;
	for( auto it = begin( m_transitions ); it != end( m_transitions ); ++it )
	{
		( *it )->Link( *this );
		auto mask = ( *it )->GetVariableMask();
		if( mask == 0 )
		{
			hasMask = false;
		}
		else
		{
			m_transitionVariableMask |= mask;
		}
	}
	if( !hasMask )
	{
		m_transitionVariableMask = 0;
	}

	for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
	{
		( *it )->Link( *stateMachine.GetController() );
	}
	if( m_finalizer )
	{
		m_finalizer->Link( *stateMachine.GetController() );
	}
}

void Tr2StateMachineState::UpdateVariableMask() const
{
	m_transitionVariableMask = 0;
	bool hasMask = true;
	for( auto it = begin( m_transitions ); it != end( m_transitions ); ++it )
	{
		auto mask = ( *it )->GetVariableMask();
		if( mask == 0 )
		{
			hasMask = false;
		}
		else
		{
			m_transitionVariableMask |= mask;
		}
	}
	if( !hasMask )
	{
		m_transitionVariableMask = 0;
	}
}

void Tr2StateMachineState::Unlink( UnlinkReason reason )
{
	if( !m_stateMachine )
	{
		return;
	}
	if( reason != UnlinkReason::DELETING )
	{
		Stop();
	}
	m_stateMachine = nullptr;
	for( auto it = begin( m_transitions ); it != end( m_transitions ); ++it )
	{
		( *it )->Unlink();
	}
	for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
	{
		( *it )->Unlink();
	}
	if( m_finalizer )
	{
		m_finalizer->Unlink();
	}
	m_transitionVariableMask = 0;
}

Tr2StateMachineState* Tr2StateMachineState::Update( uint64_t variableDirtyMask )
{
	if( !m_isActive )
	{
		return nullptr;
	}
	if( m_isFinalizing )
	{
		auto next = GetNextState();

		if( !next )
		{
			m_isActive = false;
			Start();
		}
		if( !m_finalizer || m_finalizer->CanTransition( *m_stateMachine->GetController() ) )
		{
			return next;
		}
		return nullptr;
	}
	// If an action has vetoed transition before, we can't rely on variable dirty state anymore
	if( m_hasBeenVetoed )
	{
		variableDirtyMask = 0xffffffffffffffffull;
	}
	if( m_transitionVariableMask != 0 && ( ( m_transitionVariableMask & variableDirtyMask ) == 0 ) )
	{
		return nullptr;
	}
	for( auto it = begin( m_transitions ); it != end( m_transitions ); ++it )
	{
		if( ( *it )->CanActivate( variableDirtyMask ) && ( *it )->GetDestination() )
		{
			for( auto ai = begin( m_actions ); ai != end( m_actions ); ++ai )
			{
				if( !( *ai )->CanTransition() )
				{
					m_hasBeenVetoed = true;
					return nullptr;
				}
			}

			Stop();
			if( m_isFinalizing )
			{
				return nullptr;
			}
			return ( *it )->GetDestination();
		}
	}
	return nullptr;
}

void Tr2StateMachineState::RebaseSimTime( Be::Time diff )
{
	for( auto action = m_actions.begin(); action != m_actions.end(); ++action )
	{
		( *action )->RebaseSimTime( diff );
	}
}

Tr2StateMachineState* Tr2StateMachineState::GetNextState() const
{
	for( auto it = begin( m_transitions ); it != end( m_transitions ); ++it )
	{
		if( ( *it )->CanActivate( 0xffffffffffffffffull ) && ( *it )->GetDestination() )
		{
			return ( *it )->GetDestination();
		}
	}
	return nullptr;
}

const Tr2StateMachine* Tr2StateMachineState::GetStateMachine() const
{
	return m_stateMachine;
}

const std::string& Tr2StateMachineState::GetName() const
{
	return m_name;
}

void Tr2StateMachineState::Start()
{
	if( m_isActive )
	{
		return;
	}
	if( m_stateMachine )
	{
		auto owner = m_stateMachine->GetController() != nullptr ? m_stateMachine->GetController()->GetOwner() : nullptr;

		for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
		{
			ContinueOnMainThread( [_ = IRootPtr( owner ), action = ITr2ControllerActionPtr( *it ), self = Tr2StateMachineStatePtr( this )]() {
				if( self->m_stateMachine && action )
				{
					action->Start( *self->m_stateMachine->GetController() );
				}
			} );
		}
		m_isActive = true;
		m_isFinalizing = false;
		m_hasBeenVetoed = false;
	}
}

void Tr2StateMachineState::Stop()
{
	if( !m_isActive || m_isFinalizing )
	{
		return;
	}
	if( m_stateMachine )
	{
		auto owner = m_stateMachine->GetController() != nullptr ? m_stateMachine->GetController()->GetOwner() : nullptr;
		for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
		{
			ContinueOnMainThread( [_ = IRootPtr( owner ), action = ITr2ControllerActionPtr( *it ), self = Tr2StateMachineStatePtr( this )]() {
				if( self->m_stateMachine && action )
				{
					action->Stop( *self->m_stateMachine->GetController() );
				}
			} );
		}
		if( m_finalizer )
		{
			if( !m_finalizer->CanTransition( *m_stateMachine->GetController() ) )
			{
				m_isFinalizing = true;
				return;
			}
		}
	}
	m_isActive = false;
}

IRoot* Tr2StateMachineState::GetStateMachinePtr() const
{
	return m_stateMachine ? m_stateMachine->GetRawRoot() : nullptr;
}
