// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerInterval.h"
#include "include/TriMath.h"

EveDistributionSpawnerInterval::EveDistributionSpawnerInterval( IRoot* lockobj ) :
	m_localTimer( 0.f ),
	m_delayBeforeInitialSpawn( 0.f ),
	m_numTriggered( 0 ),
	m_numberOfTriggers( 0 ),
	m_delayBetweenRepeats( 1.f ),
	m_useRandomStartOffset( true ),
	m_currentRandomizedIntervalDelta( 0.f ),
	m_maxRandomizedIntervalDelta( 0.f )
{
}

void EveDistributionSpawnerInterval::Reset( const std::vector<InitialPlacement>& placements )
{
	Restart();
}

void EveDistributionSpawnerInterval::Restart()
{
	m_localTimer = m_useRandomStartOffset ? TriRand() * m_delayBetweenRepeats : 0.f;
	m_localTimer -= m_delayBeforeInitialSpawn;
	m_numTriggered = 0;
}

void EveDistributionSpawnerInterval::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
	if( m_numberOfTriggers == 0 || m_numTriggered < m_numberOfTriggers )
	{
		m_localTimer += updateContext.GetDeltaT();
		if( m_localTimer > m_delayBetweenRepeats )
		{
			owner.AddEntities( 1 );
			m_numTriggered++;
			m_localTimer = m_maxRandomizedIntervalDelta - 2.f * TriRand() * m_maxRandomizedIntervalDelta;
		}
	}
}
