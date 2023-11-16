#include "StdAfx.h"
#include "Wander.h"
#include "Include/TriMath.h"
#include "include/TriQuaternion.h"

Wander::Wander(IRoot* lockobj) :
	m_enabled( true ),
	m_weightWander( 240.f ),
	rand1(0.2),
	rand2(0.8),
	rand3(1.2),
	m_freq(2),
	m_priority( LEAST_PRIORITY )
{
}

Wander::~Wander()
{
}

int Wander::GetProcessPriority()
{
	return m_priority;
}

std::vector<Vector3> Wander::CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> forceVectors;
	if( !m_enabled )
	{
		return forceVectors;
	}

	for( auto agent = agents.begin(); agent != agents.end(); ++agent )
	{
		float seed = agent->lifetime + agent->id;

		Vector3 p = Vector3( seed * rand1, seed * rand2, seed * rand3 ) * m_freq;

		Vector3 force( float( PerlinNoise1D( p.x, 2, 1, 1 ) ), float( PerlinNoise1D( p.y, 2, 1, 1 ) ), float( PerlinNoise1D( p.z, 2, 1, 1 ) ) );
		Vector3 forceOffset = Normalize( force ) * group.GetBoundingSphereRadius();
		forceVectors.push_back( agent->position + forceOffset );
		force *= m_weightWander;
		forceVectors.push_back( force );
		agent->acceleration += force;
	}
	return forceVectors;
}
