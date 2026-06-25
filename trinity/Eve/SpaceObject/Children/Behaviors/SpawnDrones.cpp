// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "SpawnDrones.h"
#include "ProcessLifetime.h"

SpawnDrones::SpawnDrones( IRoot* lockobj ) :
	m_enabled( true ),
	m_addByCount( false ),
	m_addOnGrid( false ),
	m_regenerateDrones( true ),
	m_seconds( -1 ),
	m_time( 0.f ),
	m_count( 1 ),
	m_spawnPosition( 0, 0, 0 ),
	m_gridInfo( 1, 1, 1, 10 ),
	m_gridSpacing( 0, 0, 0 ),
	m_gridFullnessFactor( 1.f )
{
}

SpawnDrones::~SpawnDrones()
{
}

void SpawnDrones::UpdateGrid( BehaviorGroup& group )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// for behaviors to work we always have to add one decoy drone, delete that one so he doesn't mess up the cube
	for( unsigned int i = 0; i < group.GetCount(); i++ )
	{
		group.SetCount( 0 );
	}

	Vector3 startPos = group.m_spawnPosition;
	Vector3 position = startPos;
	int xCount = int( m_gridInfo.x );
	int yCount = int( m_gridInfo.y );
	int zCount = int( m_gridInfo.z );

	Vector3 distBetween = Vector3( m_gridInfo.w, m_gridInfo.w, m_gridInfo.w );
	if( LengthSq( m_gridSpacing ) != 0.f )
	{
		distBetween = m_gridSpacing;
	}
	float fullnessFactor = m_gridFullnessFactor;

	std::vector<Vector3> spawnPoints;
	spawnPoints.reserve( xCount * yCount * zCount );

	for( int i = 0; i < zCount; ++i )
	{
		for( int j = 0; j < yCount; ++j )
		{
			for( int k = 0; k < xCount; ++k )
			{
				// sometimes we may want to randomize the fullness
				if( m_gridFullnessFactor == -1 )
				{
					fullnessFactor = TriRand();
				}
				// the higher the gridRandomFactor the more full the grid should be
				if( TriRand() <= fullnessFactor )
				{
					spawnPoints.push_back( position );
				}

				position.x += distBetween.x;
			}
			position.y += distBetween.y;
			position.x = startPos.x;
		}
		position.z += distBetween.z;
		position.y = startPos.y;
	}

	if( !spawnPoints.empty() )
	{
		group.AddAgents( spawnPoints );
	}
	m_regenerateDrones = false;
	// reset the spawn position to zero because otherwise it will offset every group on next spawn
	group.m_spawnPosition = Vector3( 0, 0, 0 );
}

std::vector<Vector3> SpawnDrones::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> noNeedToReturnForces;
	if( !m_enabled )
	{
		return noNeedToReturnForces;
	}

	if( m_addOnGrid && m_regenerateDrones )
	{
		UpdateGrid( group );
		return noNeedToReturnForces;
	}

	// If m_addByCount is toggled on the behavior adds agents by count
	if( m_addByCount == true )
	{
		std::vector<Vector3> spawnPoints;
		spawnPoints.resize( m_count, group.m_spawnPosition );
		group.AddAgents( spawnPoints );
		m_addByCount = false;
	}

	if( m_seconds <= 0.0 )
	{
		return noNeedToReturnForces;
	}

	m_time += deltaTime;

	if( m_time > m_seconds && m_seconds >= 0.0 )
	{
		auto behavior = group.GetBehaviorByName( "ProcessLifetime" );
		if( behavior != nullptr )
		{
			auto processLifetime = dynamic_cast<ProcessLifetime*>( behavior );
			if( processLifetime )
			{
				std::vector<Vector3> spawnPoints = processLifetime->GetEntrancePoints();
				if( !spawnPoints.empty() )
				{
					const auto randomNbr = rand() % spawnPoints.size();
					m_spawnPosition = spawnPoints.at( randomNbr );
				}
			}
		}

		group.m_spawnPosition = m_spawnPosition;

		std::vector<Vector3> spawnPoints;
		spawnPoints.resize( m_count, group.m_spawnPosition );
		group.AddAgents( spawnPoints );

		m_time = 0.0f;
	}

	return noNeedToReturnForces;
}

void SpawnDrones::GridToggleReset()
{
	m_regenerateDrones = true;
}