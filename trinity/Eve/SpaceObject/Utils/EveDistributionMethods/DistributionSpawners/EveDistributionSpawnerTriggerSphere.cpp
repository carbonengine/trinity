// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerTriggerSphere.h"
#include "include/TriMath.h"

EveDistributionSpawnerTriggerSphere::EveDistributionSpawnerTriggerSphere( IRoot* lockobj ) :
	m_sphereOffset( 0.f, 0.f, 0.f ),
	m_delayBeforeActivation( 0.f ),
	m_playDuration( 1.f ),
	m_currentPlayTime( 0.f ),
	m_triggerChance( 1.f ),
	m_currentTrigger( 0 ),
	m_startSequenceAtFirstTrigger( true ),
	m_reverseSphereAnimation( false )
{
}

void EveDistributionSpawnerTriggerSphere::Reset( const std::vector<InitialPlacement>& placements )
{
	if( placements.empty() )
	{
		return;
	}

	m_distSortedIndexes.clear();
	m_distSortedIndexes.reserve( placements.size() );

	for( auto& placement : placements )
	{
		float dist = Length( placement.placement.initialTranslation - m_sphereOffset );
		m_distSortedIndexes.push_back( std::make_pair( dist, placement.placement.uniqueID ) );
	}

	std::sort( m_distSortedIndexes.begin(), m_distSortedIndexes.end(), []( auto& a, auto& b ) { return a.first < b.first; } );

	float minDist = m_startSequenceAtFirstTrigger ? m_distSortedIndexes.front().first : 0.f;
	float maxDist = max( 1.f, m_distSortedIndexes.back().first - minDist );

	for( auto& trigger : m_distSortedIndexes )
	{
		trigger.first = ( trigger.first - minDist ) / maxDist;
	}

	Restart();
}

void EveDistributionSpawnerTriggerSphere::Restart()
{
	m_currentTrigger = m_reverseSphereAnimation ? uint32_t( m_distSortedIndexes.size() - 1 ) : 0;
	m_currentPlayTime = 0.f;
}

void EveDistributionSpawnerTriggerSphere::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
	if( m_currentPlayTime >= m_playDuration + m_delayBeforeActivation )
	{
		return;
	}

	m_currentPlayTime += updateContext.GetDeltaT();

	if( m_currentPlayTime < m_delayBeforeActivation )
	{
		return;
	}

	float normalizedPlayTime = ( m_currentPlayTime - m_delayBeforeActivation ) / max( 0.01f, m_playDuration );

	if( !m_reverseSphereAnimation )
	{
		while( normalizedPlayTime > m_distSortedIndexes[m_currentTrigger].first )
		{
			if( TriRand() < m_triggerChance )
			{
				owner.TriggerEntityByID( m_distSortedIndexes[m_currentTrigger].second );
			}
			m_currentTrigger++;
			if( m_currentTrigger >= uint32_t( m_distSortedIndexes.size() ) )
			{
				m_currentPlayTime = m_playDuration + m_delayBeforeActivation;
				break;
			}
		}
	}
	else
	{
		while( ( 1.f - normalizedPlayTime ) < m_distSortedIndexes[m_currentTrigger].first )
		{
			if( TriRand() < m_triggerChance )
			{
				owner.TriggerEntityByID( m_distSortedIndexes[m_currentTrigger].second );
			}

			if( m_currentTrigger == 0 )
			{
				m_currentPlayTime = m_playDuration + m_delayBeforeActivation;
				break;
			}
			m_currentTrigger--;
		}
	}
}
