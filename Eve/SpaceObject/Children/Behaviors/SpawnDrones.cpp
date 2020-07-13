#include "StdAfx.h"
#include "SpawnDrones.h"
#include "ProcessLifetime.h"

SpawnDrones::SpawnDrones( IRoot* lockobj ) :
	m_seconds( 10.f ),
	m_time( 0.f ),
	m_spawnPosition( 0, 0, 0 )
{
}

SpawnDrones::~SpawnDrones()
{
}

std::vector<Vector3> SpawnDrones::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	m_time += deltaTime;

	if( m_time > m_seconds )
	{
		auto behavior = group.GetBehaviorByName( "ProcessLifetime" );
		if( behavior != nullptr )
		{
			auto processLifetime = dynamic_cast<ProcessLifetime*> ( behavior );
			if( processLifetime )
			{
				std::vector<Vector3> spawnPoints = processLifetime->GetPotentialPoints();
				if( !spawnPoints.empty() )
				{
					const auto randomNbr = rand() % spawnPoints.size();
					m_spawnPosition = spawnPoints.at( randomNbr );
				}
			}
		}

		group.m_spawnPosition = m_spawnPosition;

		group.AddAgent();
		m_time = 0.0f;
	}

	std::vector<Vector3> noNeedToReturnForces;
	return noNeedToReturnForces;
}
