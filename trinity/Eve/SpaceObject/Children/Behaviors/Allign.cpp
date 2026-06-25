// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Allign.h"


Allign::Allign( IRoot* lockobj ) :
	m_enabled( true ),
	m_behaviorWeight( 1 ),
	m_visionRange( 75 ),
	m_frameCounter( 0 ),
	m_framesBetweenUpdates( 61 ),
	m_priority( LEAST_PRIORITY )
{
	m_lastPullForces.clear();
}

Allign::~Allign()
{
}

int Allign::GetProcessPriority()
{
	return m_priority;
}

std::vector<Vector3> Allign::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	std::vector<Vector3> returnForces;
	if( !m_enabled )
	{
		return returnForces;
	}

	int c = 0;
	if( m_frameCounter == 0 )
	{
		m_lastPullForces.clear();
		for( auto agent = agents.begin(); agent != agents.end(); ++agent, c++ )
		{
			if( dronesInSearchRadius[c].empty() )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				continue;
			}

			Vector3 groupDirection = Vector3( 0, 0, 0 );
			for( auto a = dronesInSearchRadius[c].begin(); a != dronesInSearchRadius[c].end(); ++a )
			{
				if( ( *a )->id == agent->id )
				{
					continue;
				}
				groupDirection += ( *a )->acceleration;
			}

			Vector3 pullVector = Normalize( groupDirection ) * m_behaviorWeight;

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

float Allign::GetBehaviorSearchRadius()
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

void Allign::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "BehaviorVisionRanges" );
}

void Allign::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "BehaviorVisionRanges" ) )
	{
		if( m_visionRange <= 0 )
			return;
		float lengthLerp = min( 1.f, max( 0.f, m_visionRange / 1500 ) );
		for( auto agent = agents.begin(); agent != agents.end(); ++agent )
		{

			renderer.DrawSphere( this, TranslationMatrix( agent->position ) * parentWorldLocation, m_visionRange, 6, ITr2DebugRenderer2::Wireframe, Lerp( Color( 0xffffffff ), Color( 0xff1111ff ), lengthLerp ) );
		}
	}
}