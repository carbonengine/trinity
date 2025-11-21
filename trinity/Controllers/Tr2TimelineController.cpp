////////////////////////////////////////////////////////////
//
//    Created:   August 2025
//    Copyright: CCP 2025
//

#include "StdAfx.h"
#include "Tr2TimelineController.h"
#include "Tr2ControllerFloatVariable.h"
#include "Tr2ControllerEventHandler.h"
#include "../Include/ITr2Updateable.h"
#include "../Tr2ExpressionTermInfo.h"
#include <ScopedBlockTrap.h>


CCP_STATS_DECLARED_ELSEWHERE( controllerLinkTime );
CCP_STATS_DECLARED_ELSEWHERE( controllerLinkCount );
CCP_STATS_DECLARED_ELSEWHERE( controllerUpdateCount );
CCP_STATS_DECLARED_ELSEWHERE( controllerUpdateTime );
CCP_STATS_DECLARED_ELSEWHERE( controllerUpdateablesTime );

extern CcpMutex g_controllerMutex;


namespace
{

BlueStructureDefinition Tr2TimelineEntryDef[] = {
	{ "startTime", Be::FLOAT32_1, 0 },
	{ "endTime", Be::FLOAT32_1, 4 },
	{ "trackID", Be::UINT32_1, 8 },
	{ 0 }
};

bool InRange( float time, const Tr2TimelineEntry& range )
{
	return time >= range.startTime && time < range.endTime;
}

}

Tr2TimelineController::Tr2TimelineController( IRoot* lockobj ) :
	PARENTLOCK( m_actions ),
	PARENTLOCK( m_entries ),
	PARENTLOCK( m_variables ),
	PARENTLOCK( m_eventHandlers )
{
	m_entries.SetStructureDefinition( Tr2TimelineEntryDef );
	BeOS->RegisterForSimTimeRebase( this );
}

Tr2TimelineController::~Tr2TimelineController()
{
	BeOS->UnregisterForSimTimeRebase( this );
}


void Tr2TimelineController::Link( IRoot& owner )
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
		for( auto& var : m_variables )
		{
			var->SetDestinationBuffer( reinterpret_cast<float*>( m_variableData.get() + offset ) );
			offset += sizeof( float );
			var->SetDirtyMask( nullptr, 0 );
		}

		m_owner = &owner;
		for( auto& action : m_actions )
		{
			action->Link( *this );
		}
		for( auto& eventHandler : m_eventHandlers )
		{
			eventHandler->Link( *this );
		}
	}
}

void Tr2TimelineController::Unlink()
{
	if( !m_owner )
	{
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );

	Stop();
	for( auto& var : m_variables )
	{
		var->SetDestinationBuffer( nullptr );
		var->SetDirtyMask( nullptr, 0 );
	}
	for( auto& action : m_actions )
	{
		action->Unlink();
	}
	for( auto& eventHandler : m_eventHandlers )
	{
		eventHandler->Unlink();
	}
	m_bindingPathRoots.clear();
	m_owner = nullptr;
}

bool Tr2TimelineController::IsLinked() const
{
	return m_owner != nullptr;
}

void Tr2TimelineController::Start()
{
	ScopedBlockTrap blockTrap;

	if( m_isActive )
	{
		Stop();
	}
	m_isActive = true;
	m_lastUpdateTime = BeOS->GetCurrentFrameTime();

	CcpAutoMutex lock( g_controllerMutex );

	for( size_t i = 0; i < m_actions.size(); ++i )
	{
		auto action = m_actions[i];
		auto& entry = m_entries[i];
		if( InRange( m_time, entry ) && IsActionEnabled( i ) )
		{
			action->Start( *this );
		}
	}
}

void Tr2TimelineController::Stop()
{
	if( !m_isActive )
	{
		return;
	}
	CcpAutoMutex lock( g_controllerMutex );
	for( size_t i = 0; i < m_actions.size(); ++i )
	{
		auto action = m_actions[i];
		auto& entry = m_entries[i];
		if( InRange( m_time, entry ) && IsActionEnabled( i ) )
		{
			action->Stop( *this );
		}
	}
	m_isActive = false;
	m_time = 0;
}

void Tr2TimelineController::Update()
{
	if( !m_isActive )
	{
		return;
	}

	{
		CCP_STATS_INC( controllerUpdateCount );
		CCP_STATS_SCOPED_TIME( controllerUpdateTime );

		auto simTime = BeOS->GetCurrentFrameTime();
		auto dt = TimeAsFloat( simTime - m_lastUpdateTime ) * m_timeScale;
		m_lastUpdateTime = simTime;
		if( !m_isPaused )
		{
			CcpAutoMutex lock( g_controllerMutex );

			auto oldTime = m_time;
			m_time += dt;

			for( size_t i = 0; i < m_actions.size(); ++i )
			{
				if( !IsActionEnabled( i ) )
				{
					continue;
				}
				auto action = m_actions[i];
				auto& entry = m_entries[i];
				auto wasActive = InRange( oldTime, entry );
				auto nowActive = InRange( m_time, entry );
				if( wasActive && !nowActive )
				{
					action->Stop( *this );
				}
				else if( !wasActive && nowActive )
				{
					action->Start( *this );
				}
				else if( InRange( entry.startTime, { oldTime, m_time } ) && InRange( entry.endTime, { oldTime, m_time } ) )
				{
					action->Start( *this );
					action->Stop( *this );
				}
			}
		}

		if( !m_updateables.empty() )
		{
			CCP_STATS_SCOPED_TIME( controllerUpdateablesTime );

			auto realTime = BeOS->GetActualTime();

			CcpAutoMutex lock( g_controllerMutex );

			for( auto& updatable : m_updateables )
			{
				updatable->Update( realTime, simTime );
			}
		}
	}
}

void Tr2TimelineController::SetVariable( const char* name, float value )
{
	for( auto& var : m_variables )
	{
		if( var->GetName() == name )
		{
			var->SetValue( value );
			return;
		}
	}
}

void Tr2TimelineController::HandleEvent( const char* eventName )
{
	if( !m_isActive )
	{
		return;
	}
	for( auto& handler : m_eventHandlers )
	{
		if( strcmp( eventName, handler->GetName() ) == 0 )
		{
			handler->Execute( *this );
		}
	}
}

IRoot* Tr2TimelineController::GetOwner() const
{
	return m_owner;
}

void Tr2TimelineController::Callback( BlueSharedString callbackName )
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

void Tr2TimelineController::RegisterUpdateable( ITr2Updateable& updateable )
{
	auto found = std::find( begin( m_updateables ), end( m_updateables ), &updateable );
	if( found == end( m_updateables ) )
	{
		m_updateables.push_back( &updateable );
	}
}

void Tr2TimelineController::UnRegisterUpdateable( ITr2Updateable& updateable )
{
	auto found = std::find( begin( m_updateables ), end( m_updateables ), &updateable );
	if( found != end( m_updateables ) )
	{
		m_updateables.erase( found );
	}
}

const std::vector<std::pair<std::string, IRoot*>>& Tr2TimelineController::GetBindingPathRoots() const
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

std::optional<float> Tr2TimelineController::GetFloatVariableByName( const char* name ) const
{
	for( auto& var : m_variables )
	{
		if( var->GetName() == name )
		{
			return var->GetValue();
		}
	}
	return std::nullopt;
}

void Tr2TimelineController::GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& out ) const
{
	for( auto it = begin( m_variables ); it != end( m_variables ); ++it )
	{
		out.push_back( Tr2ExpressionTermInfo::Variable( "Variables", ( *it )->GetName().c_str(), "controller variable" ) );
	}
}

CcpParser::VariableView Tr2TimelineController::GetVariableView() const
{
	return m_variableView;
}

void* Tr2TimelineController::GetVariableBuffer() const
{
	return m_variableData.get();
}

void Tr2TimelineController::EnsureTempArenaSize( size_t size ) const
{
	if( m_tempArena.size() < size )
	{
		m_tempArena.resize( "", size );
	}
}

void* Tr2TimelineController::GetTempArena() const
{
	return m_tempArena.get();
}

void Tr2TimelineController::OnSimClockRebase( Be::Time oldTime, Be::Time newTime )
{
	Be::Time diff = newTime - oldTime;
	m_lastUpdateTime += diff;

	for( auto& action : m_actions )
	{
		action->RebaseSimTime( diff );
	}
}

size_t Tr2TimelineController::GetActionCount() const
{
	return m_actions.size();
}

BlueStdResult Tr2TimelineController::GetAction( size_t index, ITr2ControllerActionPtr& action )
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	action = m_actions[index];
	return {};
}

BlueStdResult Tr2TimelineController::GetActionStartTime( size_t index, float& value ) const
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	value = m_entries[index].startTime;
	return {};
}

BlueStdResult Tr2TimelineController::GetActionEndTime( size_t index, float& value ) const
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	value = m_entries[index].endTime;
	return {};
}

BlueStdResult Tr2TimelineController::GetActionTrackID( size_t index, uint32_t& trackID ) const
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	trackID = m_entries[index].trackID;
	return {};
}

BlueStdResult Tr2TimelineController::SetActionStartTime( size_t index, float startTime )
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	if( m_isActive && IsActionEnabled( index ) )
	{
		auto action = m_actions[index];
		auto& entry = m_entries[index];
		auto wasActive = InRange( m_time, entry );
		auto nowActive = InRange( m_time, { startTime, entry.endTime } );
		if( wasActive && !nowActive )
		{
			CcpAutoMutex lock( g_controllerMutex );
			action->Stop( *this );
		}
		else if( !wasActive && nowActive )
		{
			CcpAutoMutex lock( g_controllerMutex );
			action->Start( *this );
		}
	}
	m_entries[index].startTime = startTime;
	return {};
}

BlueStdResult Tr2TimelineController::SetActionEndTime( size_t index, float endTime )
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	if( m_isActive && IsActionEnabled( index ) )
	{
		CcpAutoMutex lock( g_controllerMutex );
		auto action = m_actions[index];
		auto& entry = m_entries[index];
		auto wasActive = InRange( m_time, entry );
		auto nowActive = InRange( m_time, { entry.startTime, endTime } );
		if( wasActive && !nowActive )
		{
			action->Stop( *this );
		}
		else if( !wasActive && nowActive )
		{
			action->Start( *this );
		}
	}
	m_entries[index].endTime = endTime;
	return {};
}

BlueStdResult Tr2TimelineController::SetActionTrackID( size_t index, uint32_t trackID )
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	auto wasActive = IsActionEnabled( index );
	m_entries[index].trackID = trackID;
	auto nowActive = IsActionEnabled( index );
	if( m_isActive && wasActive != nowActive )
	{
		auto action = m_actions[index];
		auto& entry = m_entries[index];
		if( InRange( m_time, entry ) )
		{
			CcpAutoMutex lock( g_controllerMutex );
			if( nowActive )
			{
				action->Start( *this );
			}
			else
			{
				action->Stop( *this );
			}
		}
	}
	return {};
}

void Tr2TimelineController::AddAction( ITr2ControllerAction* action, float startTime, float endTime, uint32_t trackID )
{
	if( !action )
	{
		return;
	}

	m_actions.Append( action );

	if ( m_owner )
	{
		action->Link( *this );
	}
	Tr2TimelineEntry entry = { startTime, endTime, trackID };
	m_entries.Append( &entry );

	if( m_isActive )
	{
		if( InRange( m_time, entry ) && IsActionEnabled( m_entries.size() - 1 ) )
		{
			CcpAutoMutex lock( g_controllerMutex );
			action->Start( *this );
		}
	}
}

BlueStdResult Tr2TimelineController::RemoveAction( size_t index )
{
	if( index >= m_actions.size() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Index out of range" );
	}
	if( m_owner )
	{
		if( m_isActive && IsActionEnabled( index ) )
		{
			auto action = m_actions[index];
			auto& entry = m_entries[index];
			if( InRange( m_time, entry ) )
			{
				CcpAutoMutex lock( g_controllerMutex );
				action->Stop( *this );
			}
		}
		m_actions[index]->Unlink();
	}
	m_actions.Remove( index );
	m_entries.Remove( index );
	return {};
}

bool Tr2TimelineController::IsActionEnabled( size_t index ) const
{
	return find( begin( m_disabledTracks ), end( m_disabledTracks ), m_entries[index].trackID ) == end( m_disabledTracks );
}

bool Tr2TimelineController::IsTrackEnabled( uint32_t trackID ) const
{
	return find( begin( m_disabledTracks ), end( m_disabledTracks ), trackID ) == end( m_disabledTracks );
}

void Tr2TimelineController::EnableTrack( uint32_t trackID, bool enable )
{
	auto found = find( begin( m_disabledTracks ), end( m_disabledTracks ), trackID );
	if( enable )
	{
		if( found != end( m_disabledTracks ) )
		{
			m_disabledTracks.erase( found );
		}
		else
		{
			return;
		}
	}
	else
	{
		if( found == end( m_disabledTracks ) )
		{
			m_disabledTracks.push_back( trackID );
		}
		else
		{
			return;
		}
	}
	if( m_isActive )
	{
		CcpAutoMutex lock( g_controllerMutex );
		for( size_t i = 0; i < m_actions.size(); ++i )
		{
			auto action = m_actions[i];
			auto& entry = m_entries[i];
			if ( InRange( m_time, entry ) && entry.trackID == trackID )
			{
				if( enable )
				{
					action->Start( *this );
				}
				else
				{
					action->Stop( *this );
				}
			}
		}
	}
}

void Tr2TimelineController::RegisterCallback( const BlueSharedString& callbackName, const BlueScriptCallback& callback )
{
	m_callbacks.push_back( { callbackName, callback } );
}

void Tr2TimelineController::ClearCallbacks()
{
	m_callbacks.clear();
}

float Tr2TimelineController::GetTime() const
{
	return m_time;
}

void Tr2TimelineController::SetTime( float time )
{
	if( !m_isActive )
	{
		return;
	}
	if( time == m_time )
	{
		return;
	}

	auto oldTime = m_time;
	m_time = time;

	CcpAutoMutex lock( g_controllerMutex );
	for( size_t i = 0; i < m_actions.size(); ++i )
	{
		if( !IsActionEnabled( i ) )
		{
			continue;
		}
		auto action = m_actions[i];
		auto& entry = m_entries[i];
		auto wasActive = InRange( oldTime, entry );
		auto nowActive = InRange( m_time, entry );
		if( wasActive && !nowActive )
		{
			action->Stop( *this );
		}
		else if( !wasActive && nowActive )
		{
			action->Start( *this );
		}
	}
}

void Tr2TimelineController::Pause()
{
	m_isPaused = true;
}

void Tr2TimelineController::Resume()
{
	m_isPaused = false;
}

void Tr2TimelineController::ReLink()
{
	if( !m_owner )
	{
		return;
	}
	auto owner = m_owner;
	Link( *owner );
}
