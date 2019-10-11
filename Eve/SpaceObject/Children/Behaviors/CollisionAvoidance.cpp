#include "StdAfx.h"
#include "CollisionAvoidance.h"

CollisionAvoidance::CollisionAvoidance( IRoot* lockobj ) :
	m_collisionAvoidanceScalar( 10.f ),
	m_collisionStrength( 0.f ) 
{
}

CollisionAvoidance::~CollisionAvoidance()
{
}

std::vector<Vector3> CollisionAvoidance::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector < std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	std::vector<Vector3> forceVectors;
	for( auto agent = agents.begin(); agent != agents.end(); ++agent )
	{
		Vector3 avoidanceForce( 0, 0, 0 );
		for( auto exclusionVolume = group.m_exclusionVolumes.begin(); exclusionVolume != group.m_exclusionVolumes.end(); ++exclusionVolume )
		{
			float intensity = ( *exclusionVolume )->GetIntensity( agent->position );
			Vector3 pos = ( *exclusionVolume )->GetBoundingSphere().GetXYZ();

			// This is when drones realize they're in the outer radius
			if( intensity > 0.0f )
			{
				avoidanceForce = agent->position - pos;
				Vector3 force = Normalize( avoidanceForce );
				Vector3 velocity = agent->velocity;
				velocity = Normalize( velocity );
				float dotProduct = Dot( force, velocity );

				if( dotProduct < 0.f )
				{
					m_collisionStrength = ( intensity * m_collisionAvoidanceScalar * -dotProduct);
				}
			}

			Vector3 forceOffset = Normalize( avoidanceForce ) * group.GetBoundingSphereRadius();
			forceVectors.push_back( agent->position + forceOffset );

			avoidanceForce = Normalize( avoidanceForce )  * ( m_collisionStrength );
			agent->acceleration += avoidanceForce;

			forceVectors.push_back( avoidanceForce );
		}
	}
	return forceVectors;
}

void CollisionAvoidance::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
}