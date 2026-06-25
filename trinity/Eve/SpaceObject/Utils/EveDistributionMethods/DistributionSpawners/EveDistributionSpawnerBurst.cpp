// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerBurst.h"
#include "include/TriMath.h"

EveDistributionSpawnerBurst::EveDistributionSpawnerBurst( IRoot* lockobj ) :
	m_delayBeforeInitialBurst( 0.f ),
	m_completeness( 1.f ),
	m_additionalTriggersPerBurst( 0 ),
	m_localTimer( 0.f )
{
}

void EveDistributionSpawnerBurst::Reset( const std::vector<InitialPlacement>& placements )
{
	Restart();
}

void EveDistributionSpawnerBurst::Restart()
{
	m_localTimer = 0.f;
}

void EveDistributionSpawnerBurst::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
	if( m_localTimer == -1.f )
	{
		return;
	}
	else if( m_localTimer < m_delayBeforeInitialBurst )
	{
		m_localTimer += updateContext.GetDeltaT();
	}
	else
	{
		uint32_t availableTriggers = owner.GetFreePlacementCount();
		uint32_t numTriggers = uint32_t( m_completeness * availableTriggers );
		numTriggers += m_additionalTriggersPerBurst;
		numTriggers = min( numTriggers, availableTriggers );
		owner.AddEntities( numTriggers );
		m_localTimer = -1.f;
	}
}
