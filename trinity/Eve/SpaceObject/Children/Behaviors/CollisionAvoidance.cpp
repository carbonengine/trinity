#include "StdAfx.h"
#include "CollisionAvoidance.h"

CollisionAvoidance::CollisionAvoidance( IRoot* lockobj ) :
	PARENTLOCK( m_exclusionVolumes ),
	m_enabled( true ),
	m_collisionAvoidanceScalar( 12.0f ),
	m_priority( LEAST_PRIORITY )
{
}

CollisionAvoidance::~CollisionAvoidance()
{
}

int CollisionAvoidance::GetProcessPriority()
{
	return m_priority;
}

std::vector<Vector3> CollisionAvoidance::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> forceVectors;
	if( !m_enabled )
	{
		return forceVectors;
	}
	for( auto agent = agents.begin(); agent != agents.end(); ++agent )
	{
		for( int i = 0; i < m_exclusionVolumes.size(); ++i )
		{
			float intensity = m_exclusionVolumes[i]->GetIntensity( agent->position );
			//we only want to continue if we are inside the outer radius
			if( intensity > 0.0 )
			{
				// get the direction AWAY from the center of the exclusion volume
				Vector3 fromTarget = agent->position - m_exclusionVolumes[i]->GetBoundingSphere().GetXYZ();

				Vector3 avoidanceForce = fromTarget * intensity;
				agent->acceleration += avoidanceForce * m_collisionAvoidanceScalar;
			}
		}
	}
	return forceVectors;
}

void CollisionAvoidance::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "ExclusionVolumes" );
}

void CollisionAvoidance::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "ExclusionVolumes" ) )
	{
		for( auto volume = m_exclusionVolumes.begin(); volume != m_exclusionVolumes.end(); ++volume )
		{
			( *volume )->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}
}