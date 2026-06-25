// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2Controller.h"
#include "Tr2StateMachine.h"
#include "Tr2ControllerFloatVariable.h"
#include "Tr2ControllerEventHandler.h"
#include "Include/ITr2Updateable.h"
#include "../Tr2ExpressionTermInfo.h"
#include "ContinueOnMainThread.h"

CCP_STATS_DECLARE( controllerUpdateTime, "Trinity/Controllers/UpdateTime", true, CST_TIME, "Cumulative per-frame time for controller update" );
CCP_STATS_DECLARE( controllerUpdateablesTime, "Trinity/Controllers/UpdateablesTime", true, CST_TIME, "Cumulative per-frame time for controller updates tick" );
CCP_STATS_DECLARE( controllerUpdateCount, "Trinity/Controllers/UpdateCount", true, CST_COUNTER_LOW, "Number of contoller Update calls per frame" );
CCP_STATS_DECLARE( controllerLinkTime, "Trinity/Controllers/LinkTime", false, CST_TIME, "Cumulative time spent in contorller Link calls" );
CCP_STATS_DECLARE( controllerLinkCount, "Trinity/Controllers/LinkCount", false, CST_COUNTER_LOW, "Number of contoller Link calls" );


CcpMutex g_controllerMutex( "", "g_controllerMutex" );

Tr2Controller::Tr2Controller( IRoot* lockobj ) :
	PARENTLOCK( m_stateMachines ),
	PARENTLOCK( m_variables ),
	PARENTLOCK( m_eventHandlers ),
	m_updateables( "Tr2Controller::m_updateables" ),
	m_dirtyVariables( 0xffffffffffffffffull ),
	m_owner( nullptr ),
	m_isActive( false ),
	m_isShared( false )
{
	m_stateMachines.SetNotify( this );
	m_variables.SetNotify( this );
	m_eventHandlers.SetNotify( this );
}

void Tr2Controller::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_stateMachines )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( m_owner )
			{
				if( Tr2StateMachinePtr stateMachine = BlueCastPtr( value ) )
				{
					stateMachine->Link( *this );
					if( m_isActive )
					{
						stateMachine->Start();
					}
				}
			}
			break;
		case BELIST_REMOVED:
			if( Tr2StateMachinePtr stateMachine = BlueCastPtr( value ) )
			{
				if( m_isActive )
				{
					stateMachine->Stop();
				}
				stateMachine->Unlink();
			}
			break;
		default:
			break;
		}
	}
	else if( list == &m_eventHandlers )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( m_owner )
			{
				if( Tr2ControllerEventHandlerPtr handler = BlueCastPtr( value ) )
				{
					handler->Link( *this );
				}
			}
			break;
		case BELIST_REMOVED:
			if( Tr2ControllerEventHandlerPtr handler = BlueCastPtr( value ) )
			{
				handler->Unlink();
			}
			break;
		default:
			break;
		}
	}
	else if( list == &m_variables )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
		case BELIST_REMOVED: {
			if( auto owner = m_owner )
			{
				Unlink();
				Link( *owner );
			}
			break;
		}
		default:
			break;
		}
	}
}

void Tr2Controller::Link( IRoot& owner )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	{

		CCP_STATS_INC( controllerLinkCount );
		CCP_STATS_SCOPED_TIME( controllerLinkTime );

		Unlink();


		CcpParser::OffsetType offset = 0;
		m_variableView.clear();
		for( auto& var : m_variables )
		{
			m_variableView.push_back( { var->GetName().c_str(), 0, offset } );
			offset += sizeof( float );
		}
		m_variableData.resize( "", offset );
		offset = 0;
		uint32_t index = 0;
		for( auto& var : m_variables )
		{
			var->SetDestinationBuffer( reinterpret_cast<float*>( m_variableData.get() + offset ) );
			offset += sizeof( float );
			if( index < 64 )
			{
				var->SetDirtyMask( &m_dirtyVariables, 1ull << index++ );
			}
			else
			{
				var->SetDirtyMask( nullptr, 0 );
			}
		}

		m_owner = &owner;
		for( auto it = begin( m_stateMachines ); it != end( m_stateMachines ); ++it )
		{
			( *it )->Link( *this );
		}
		for( auto it = begin( m_eventHandlers ); it != end( m_eventHandlers ); ++it )
		{
			( *it )->Link( *this );
		}
	}
}

void Tr2Controller::Unlink( UnlinkReason reason )
{
	if( !m_owner )
	{
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );
	if( reason != UnlinkReason::DELETING )
	{
		Stop();
	}
	for( auto& var : m_variables )
	{
		var->SetDestinationBuffer( nullptr );
		var->SetDirtyMask( nullptr, 0 );
	}
	for( auto it = begin( m_stateMachines ); it != end( m_stateMachines ); ++it )
	{
		( *it )->Unlink( reason );
	}
	for( auto it = begin( m_eventHandlers ); it != end( m_eventHandlers ); ++it )
	{
		( *it )->Unlink();
	}
	m_bindingPathRoots.clear();
	m_owner = nullptr;
}

void Tr2Controller::ReLink()
{
	if( !m_owner )
	{
		return;
	}
	auto owner = m_owner;
	Link( *owner );
}

bool Tr2Controller::IsLinked() const
{
	return m_owner != nullptr;
}

void Tr2Controller::Start()
{
	if( m_isActive )
	{
		Stop();
	}
	m_dirtyVariables = 0xffffffffffffffffull;
	for( auto it = begin( m_stateMachines ); it != end( m_stateMachines ); ++it )
	{
		( *it )->Start();
	}
	m_isActive = true;
}

void Tr2Controller::Stop()
{
	if( !m_isActive )
	{
		return;
	}
	for( auto it = begin( m_stateMachines ); it != end( m_stateMachines ); ++it )
	{
		( *it )->Stop();
	}
	m_isActive = false;
}


// param normalizedUpdateFrequency : value [0:1] on how freqeuntly the controller should be updating
void Tr2Controller::Update( float normalizedUpdateFrequency )
{
	if( !m_isActive )
	{
		return;
	}

	if( EveThrottleable::ShouldSkipUpdate( normalizedUpdateFrequency ) )
	{
		return;
	}


	auto currentTime = BeOS->GetActualTime();

	{
		CCP_STATS_INC( controllerUpdateCount );
		CCP_STATS_SCOPED_TIME( controllerUpdateTime );

		auto dirtyVariables = m_dirtyVariables;
		m_dirtyVariables = 0;

		for( auto it = begin( m_stateMachines ); it != end( m_stateMachines ); ++it )
		{
			( *it )->Update( dirtyVariables );
		}
		if( !m_updateables.empty() )
		{
			CCP_STATS_SCOPED_TIME( controllerUpdateablesTime );

			auto simTime = BeOS->GetCurrentFrameTime();

			for( auto& u : m_updateables )
			{
				ContinueOnMainThread( [updatable = ITr2UpdateablePtr( u ), currentTime, simTime]() {
					updatable->Update( currentTime, simTime );
				} );
			}
		}
	}
}

void Tr2Controller::SetVariable( const char* name, float value )
{
	if( auto var = GetVariableByName( name ) )
	{
		var->SetValue( value );
	}
}

void Tr2Controller::HandleEvent( const char* eventName )
{
	if( !m_isActive )
	{
		return;
	}
	for( auto it = begin( m_eventHandlers ); it != end( m_eventHandlers ); ++it )
	{
		auto handler = *it;
		if( strcmp( eventName, handler->GetName() ) == 0 )
		{
			handler->Execute( *this );
		}
	}
}

IRoot* Tr2Controller::GetOwner() const
{
	return m_owner;
}

Tr2ControllerFloatVariable* Tr2Controller::GetVariableByName( const char* name ) const
{
	for( auto it = begin( m_variables ); it != end( m_variables ); ++it )
	{
		if( ( *it )->GetName() == name )
		{
			return *it;
		}
	}
	return nullptr;
}

std::optional<float> Tr2Controller::GetFloatVariableByName( const char* name ) const
{
	auto var = GetVariableByName( name );
	if( var )
	{
		return var->GetValue();
	}
	return std::nullopt;
}

void Tr2Controller::GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& out ) const
{
	for( auto it = begin( m_variables ); it != end( m_variables ); ++it )
	{
		out.push_back( Tr2ExpressionTermInfo::Variable( "Variables", ( *it )->GetName().c_str(), "controller variable" ) );
	}
}

const PTr2ControllerFloatVariableVector& Tr2Controller::GetVariables() const
{
	return m_variables;
}

CcpParser::VariableView Tr2Controller::GetVariableView() const
{
	return m_variableView;
}

void* Tr2Controller::GetVariableBuffer() const
{
	return m_variableData.get();
}

void Tr2Controller::EnsureTempArenaSize( size_t size ) const
{
	if( m_tempArena.size() < size )
	{
		m_tempArena.resize( "", size );
	}
}

void* Tr2Controller::GetTempArena() const
{
	return m_tempArena.get();
}


const std::vector<std::pair<std::string, IRoot*>>& Tr2Controller::GetBindingPathRoots() const
{
	if( m_bindingPathRoots.empty() )
	{
		m_bindingPathRoots.reserve( 1 + m_variables.size() );
		if( m_owner )
		{
			m_bindingPathRoots.push_back( { "Owner", m_owner } );
		}
		for( auto& var : m_variables )
		{
			m_bindingPathRoots.push_back( { var->GetName(), var->GetRawRoot() } );
		}
	}
	return m_bindingPathRoots;
}

void Tr2Controller::RegisterUpdateable( ITr2Updateable& updateable )
{
	m_updateables.insert( &updateable );
}

void Tr2Controller::UnRegisterUpdateable( ITr2Updateable& updateable )
{
	m_updateables.erase( &updateable );
}

void Tr2Controller::Callback( BlueSharedString callbackName )
{
	if( !m_isActive || m_callbacks.empty() )
	{
		return;
	}

	for( auto callbackpair = begin( m_callbacks ); callbackpair != end( m_callbacks ); ++callbackpair )
	{
		auto pair = *callbackpair;
		if( pair.first == callbackName )
		{
			pair.second.CallVoid().ReportException();
		}
	}
}

void Tr2Controller::RegisterCallback( BlueSharedString callbackName, BlueScriptCallback callback )
{
	m_callbacks.push_back( std::pair<BlueSharedString, BlueScriptCallback>( callbackName, callback ) );
}

void Tr2Controller::ClearCallbacks()
{
	m_callbacks.clear();
}