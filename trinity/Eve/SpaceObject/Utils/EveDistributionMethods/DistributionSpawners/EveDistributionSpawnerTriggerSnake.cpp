// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerTriggerSnake.h"
#include "include/TriMath.h"

EveDistributionSpawnerTriggerSnake::EveDistributionSpawnerTriggerSnake( IRoot* lockobj ) :
	m_numDestinationsReached( 0 ),
	m_totalDestinations( 5 ),
	m_targetPoint( 0.f, 0.f, 0.f ),
	m_lastTarget( 0.f, 0.f, 0.f ),
	m_currentTravelTime( 0.f ),
	m_travelProgress( 1.f ),
	m_travelDurationToNextPoint( 1.f ),
	m_distanceToTravelTimeMultiplier( 0.f ),
	m_minTimeBetweenTriggers( 1.f ),
	m_maxTimeBetweenTriggers( 1.f ),
	m_activeTargetUniqueID( 0 )
{
}

void EveDistributionSpawnerTriggerSnake::Reset( const std::vector<InitialPlacement>& placements )
{
	if( placements.empty() )
	{
		return;
	}

	uint32_t rndIdx = uint32_t( TriRandInt( (int)placements.size() ) );
	m_targetPoint = placements[rndIdx].placement.initialTranslation;
	m_lastTarget = m_targetPoint;
	m_activeTargetUniqueID = placements[rndIdx].placement.uniqueID;
	Restart();
}

void EveDistributionSpawnerTriggerSnake::Restart()
{
	m_numDestinationsReached = -1;
	m_currentTravelTime = 0.f;
	m_travelDurationToNextPoint = 0.f;
}

void EveDistributionSpawnerTriggerSnake::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner )
{
	if( m_numDestinationsReached >= m_totalDestinations && m_totalDestinations != -1 )
	{
		return;
	}

	m_currentTravelTime += updateContext.GetDeltaT();

	if( m_travelDurationToNextPoint > 0.f )
	{
		m_travelProgress = m_currentTravelTime / m_travelDurationToNextPoint;
	}
	else
	{
		m_travelProgress = 1.f;
	}

	if( m_travelProgress >= 1.f )
	{

		owner.TriggerEntityByID( m_activeTargetUniqueID );

		m_currentTravelTime = 0.f;
		m_travelProgress = 0.f;
		m_numDestinationsReached++;
		m_travelDurationToNextPoint = Lerp( m_minTimeBetweenTriggers, m_maxTimeBetweenTriggers, (float)rand() / RAND_MAX );

		Vector3 searchPoint = Lerp( m_lastTarget, m_targetPoint, 1.3f ); // overshoot to reduce u-turns
		int32_t closetsPlace = owner.GetClosestFreePlacement( searchPoint );

		if( closetsPlace == -1 )
		{
			return;
		}

		auto pdata = owner.GetInitialPlacementData( uint32_t( closetsPlace ) );

		if( pdata != nullptr )
		{
			m_lastTarget = m_targetPoint;
			m_activeTargetUniqueID = pdata->uniqueID;
			m_targetPoint = pdata->initialTranslation;
			float dist = Length( m_targetPoint - m_lastTarget );
			m_travelDurationToNextPoint += dist * m_distanceToTravelTimeMultiplier / 100.f;
		}
	}
}
