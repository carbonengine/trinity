// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerControllerTrigger.h"

EveDistributionSpawnerControllerTrigger::EveDistributionSpawnerControllerTrigger( IRoot* lockobj ) :
	PARENTLOCK( m_distributionSpawners ),
	m_isActive( false ),
	m_restartOnReceivingValue( false ),
	m_invertReceivedValue( false ),
	m_value( 0.f )
{
}

void EveDistributionSpawnerControllerTrigger::Reset( const std::vector<InitialPlacement>& placements )
{
	Restart();
}

void EveDistributionSpawnerControllerTrigger::Restart()
{
	for( auto spawner : m_distributionSpawners )
	{
		spawner->Restart();
	}
}

bool EveDistributionSpawnerControllerTrigger::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_value ) )
	{
		if( m_invertReceivedValue )
		{
			m_isActive = 1.f - m_value > 0.f;
		}
		else
		{
			m_isActive = m_value > 0.f;
		}

		if( m_restartOnReceivingValue )
		{
			this->Restart();
		}
	}

	return true;
}

void EveDistributionSpawnerControllerTrigger::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
	if( !m_isActive )
	{
		return;
	}

	for( auto spawner : m_distributionSpawners )
	{
		spawner->UpdateSyncronous( updateContext, params, owner );
	}
}

void EveDistributionSpawnerControllerTrigger::SetControllerVariable( const char* name, float value )
{
	if( m_variableName == name )
	{
		m_value = value;

		if( m_restartOnReceivingValue )
		{
			this->Restart();
		}

		if( m_invertReceivedValue )
		{
			m_isActive = 1.f - m_value > 0.f;
		}
		else
		{
			m_isActive = m_value > 0.f;
		}
	}
}
