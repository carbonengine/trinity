// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "DroneAvoidance.h"


DroneAvoidance::DroneAvoidance( IRoot* lockobj ) :
	m_enabled( true ),
	m_behaviorWeight( 300 ),
	m_visionRange( 5 ),
	m_frameCounter( 0 ),
	m_framesBetweenUpdates( 3 ),
	m_priority( LEAST_PRIORITY )
{
	m_lastPullForces.clear();
}

DroneAvoidance::~DroneAvoidance()
{
}

int DroneAvoidance::GetProcessPriority()
{
	return m_priority;
}

std::vector<Vector3> DroneAvoidance::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> returnForces;
	if( !m_enabled )
	{
		return returnForces;
	}

	int c = 0;
	if( m_frameCounter == 0 )
	{
		m_lastPullForces.clear();
		for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++c )
		{
			if( dronesInSearchRadius.size() <= c )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				continue;
			}

			if( dronesInSearchRadius.empty() )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				return returnForces;
			}

			if( dronesInSearchRadius[c].empty() )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				continue;
			}

			if( dronesInSearchRadius[c].size() == 1 )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				continue;
			}

			Vector3 avoidanceDirection = Vector3( 0, 0, 0 );
			for( auto a = dronesInSearchRadius[c].begin(); a != dronesInSearchRadius[c].end(); ++a )
			{
				if( ( *a )->id == agent->id )
				{
					continue;
				}

				avoidanceDirection += agent->position - ( *a )->position;
			}

			if( avoidanceDirection == Vector3( 0, 0, 0 ) )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				continue;
			}

			Vector3 pullVector = ( 0.5f * Normalize( avoidanceDirection ) + 0.5f * Normalize( agent->velocity ) ) * m_behaviorWeight;

			if( LengthSq( pullVector ) > 0 )
			{
				agent->acceleration += pullVector;
				m_lastPullForces.push_back( pullVector );
			}
			else
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
			}

			if( group.m_collectForces )
			{
				Vector3 forceOffset = Normalize( pullVector ) * group.GetBoundingSphereRadius();
				returnForces.push_back( agent->position + forceOffset );
				returnForces.push_back( pullVector );
			}
		}
	}
	else
	{
		if( m_lastPullForces.empty() )
		{
			return returnForces;
		}

		for( auto agent = agents.begin(); agent != agents.end(); ++agent, c++ )
		{
			if( c >= static_cast<int>( m_lastPullForces.size() ) )
			{
				break;
			}

			agent->acceleration += m_lastPullForces[c];

			if( group.m_collectForces && m_lastPullForces[c] != Vector3( 0, 0, 0 ) )
			{
				Vector3 forceOffset = Normalize( m_lastPullForces[c] ) * group.GetBoundingSphereRadius();
				returnForces.push_back( agent->position + forceOffset );
				returnForces.push_back( m_lastPullForces[c] );
			}
		}
	}

	return returnForces;
}

float DroneAvoidance::GetBehaviorSearchRadius()
{
	if( m_frameCounter >= m_framesBetweenUpdates )
	{
		m_frameCounter = 0;
		return m_visionRange;
	}
	else
	{
		m_frameCounter++;
		return -1;
	}
}

void DroneAvoidance::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "BehaviorVisionRanges" );
}

void DroneAvoidance::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "BehaviorVisionRanges" ) )
	{
		if( m_visionRange <= 0 )
			return;
		float lengthLerp = min( 1.f, max( 0.f, m_visionRange / 300 ) );
		for( auto agent = agents.begin(); agent != agents.end(); ++agent )
		{
			renderer.DrawSphere( this, TranslationMatrix( agent->position ) * parentWorldLocation, m_visionRange, 6, Tr2DebugRenderer::Wireframe, Lerp( Color( 0xffffffff ), Color( 0xff1111ff ), lengthLerp ) );
		}
	}
}