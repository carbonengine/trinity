// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "InclusionVolume.h"


InclusionVolume::InclusionVolume( IRoot* lockobj ) :
	PARENTLOCK( m_inclusionVolumes ),
	m_enabled( true ),
	m_behaviorWeight( 60.0f ),
	m_frameCounter( 0 ),
	m_framesBetweenUpdates( 11 ),
	m_priority( LEAST_PRIORITY )
{
}

InclusionVolume::~InclusionVolume()
{
}

int InclusionVolume::GetProcessPriority()
{
	return m_priority;
}

std::vector<Vector3> InclusionVolume::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> returnForces;
	if( !m_enabled )
	{
		return returnForces;
	}
	for( auto agent = agents.begin(); agent != agents.end(); ++agent )
	{
		Vector3 force( 0, 0, 0 );
		Vector3 Nforce( 0, 0, 0 );
		float status;
		Vector3 boxPos;

		for( auto volume = m_inclusionVolumes.begin(); volume != m_inclusionVolumes.end(); ++volume )
		{
			boxPos = ( *volume )->GetBoundingSphere().center;
			status = ( *volume )->GetIntensity( agent->position );


			if( status == 1 )
			{
				force = Vector3( 0, 0, 0 );
				Nforce = Vector3( 0, 0, 0 );
				break;
			}
			else if( status > 0 )
			{
				Nforce = Normalize( boxPos - agent->position );

				force = Nforce * m_behaviorWeight;
			}
		}

		agent->acceleration += force;

		Vector3 forceOffset = Nforce * group.GetBoundingSphereRadius();
		returnForces.push_back( agent->position + forceOffset );
		returnForces.push_back( force );
	}
	return returnForces;
}

void InclusionVolume::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "InclusionVolumes" );
}

void InclusionVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "InclusionVolumes" ) )
	{
		for( auto volume = m_inclusionVolumes.begin(); volume != m_inclusionVolumes.end(); ++volume )
		{
			( *volume )->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}
}