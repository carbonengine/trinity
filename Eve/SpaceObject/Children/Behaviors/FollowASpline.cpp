#include "StdAfx.h"
#include "followASpline.h"

FollowASpline::FollowASpline( IRoot* lockobj ) :
	m_behaviorWeight( 10.f ),
	m_smoothPullFactor( 0.8 ),
	m_cornerSmoothener( 0.8 ),
	m_tunnelGroupType( OTHER_TUNNELS )
{
}

FollowASpline::~FollowASpline()
{
}

float FollowASpline::ProcessTunnelEntrances(DroneAgent& agent, std::vector<SplineTunnel>& tunnels)
{
	// not associated with a tunnel
	for ( auto tunnel = tunnels.begin(); tunnel != tunnels.end(); ++tunnel )
	{
		auto t = ( *tunnel );

		if( t.tunnelGroupID != m_tunnelGroupType )
		{
			return 0;
		}

		Vector3 dist = t.splinePoints[ 0 ].pos - agent.position;
		float length = Length( dist );
		if ( length < t.pointOfNoReturnSize )
		{
			agent.tunnelLock = t.tunnelID;
			agent.tunnelPoint = 0;
		}
		else if ( length < t.pullSize )
		{
			if ( t.pullSize == t.pointOfNoReturnSize )
			{
				continue;
			}

			// normalize the distance between outer and inner spheres to increase pull-strength
			float mod = ( length - t.pointOfNoReturnSize ) / ( t.pullSize - t.pointOfNoReturnSize );
			mod = 1 - max( 0.f, min( mod, 1.f ) );
			m_desiredVector = dist;
			return min( 1.f, max( 0.f, 1 - m_smoothPullFactor + ( m_smoothPullFactor * mod ) ) );
		}
	}
	return 1.f;
}


void FollowASpline::ProcessAssignedTunnel( DroneAgent& agent, std::vector<SplineTunnel>& tunnels, BehaviorGroup& group )
{
	auto t = tunnels[ agent.tunnelLock ];
	auto pID = agent.tunnelPoint; // Point ID

	float lengthBetweenPoints;
	Vector3 targetVector = t.splinePoints[ pID ].pos - agent.position;
	Vector3 vectorBetween = Vector3( 0, 0, 0 );

	if ( t.splinePoints[ pID ] == ( *t.splinePoints.begin() ) )
	{
		vectorBetween = t.splinePoints[ pID ].rot;
	}
	else
	{
		vectorBetween = t.splinePoints[ pID - 1 ].rot;
	}


	lengthBetweenPoints = Length( vectorBetween );

	if( 0 != lengthBetweenPoints )
	{	// an offset is added to the target point so that they don't all follow the same line
		const float dotProd = Dot( targetVector, vectorBetween );
		const Vector3 vectorProj = ( dotProd / ( lengthBetweenPoints * lengthBetweenPoints ) ) * vectorBetween;
		const Vector3 offset = ( t.cylWidth / 2 ) * Normalize( vectorProj - targetVector );
		targetVector += offset;
	}

	m_targetPointVector.push_back( targetVector + agent.position );
	

	if ( t.splinePoints[ pID ] == ( *t.splinePoints.end() ) )
	{
		m_desiredVector = t.splinePoints[ pID ].rot;

		// the Dot product is positive if the agent is facing the target point
		if ( Dot( targetVector, Vector3( agent.rotation ) ) < 0 )
		{
			agent.tunnelLock = -1;
			agent.tunnelPoint = 0;
		}
	}
	else
	{
		float lengthFromShip = Length( targetVector );

		float blendingMod = 0;

		if ( lengthBetweenPoints != 0 )
		{
			blendingMod = min( 1.f, max( 0.f, ( lengthBetweenPoints - lengthFromShip ) / lengthBetweenPoints ) );
			blendingMod = blendingMod * blendingMod;
		}
		m_cornerSmoothener = min( 1.f, max( 0.f, m_cornerSmoothener ) );
		m_desiredVector = m_cornerSmoothener * ( 1 - blendingMod ) * Normalize( targetVector ) + 
							( 1 - m_cornerSmoothener ) * blendingMod * Normalize( t.splinePoints[ pID ].rot + targetVector );
		if ( Dot( Normalize( targetVector ), Normalize( m_desiredVector ) ) < m_cornerSmoothener )
		{
			m_desiredVector = targetVector;
		}

		if ((lengthFromShip - group.GetBoundingSphereRadius()) < (t.cylWidth)/2)
		{
			agent.tunnelPoint++;
		}

		//rework into cylinder collision
		if ( lengthFromShip > ( group.GetBoundingSphereRadius() + lengthBetweenPoints * 1.5) && Dot( targetVector, vectorBetween ) < 0 )
		{
			agent.tunnelLock = -1;
			agent.tunnelPoint = 0;
		}
	}
}

std::vector<Vector3> FollowASpline::CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
                                                      BehaviorGroup& group,
                                                      EveChildBehaviorSystem& system)
{
	auto tunnels = system.GetTunnels();
	m_targetPointVector.clear();
	std::vector<Vector3> forceVectors;

	if ( tunnels.empty() )
	{
		return forceVectors;
	}

	for ( auto drone = agents.begin(); drone != agents.end(); ++drone )
	{
		m_desiredVector = Vector3( 0, 0, 0 );
		float rampingForce = 1;

		if ( drone->tunnelLock == -1 )
		{
			rampingForce = ProcessTunnelEntrances( *drone, tunnels);
		}

		// tunnelLock can change in ProcessTunnelEntrances so if/else is not equivalent
		if ( drone->tunnelLock != -1 )
		{
			ProcessAssignedTunnel( *drone, tunnels, group );
		}

		if ( m_desiredVector == Vector3( 0, 0, 0 ) )
		{
			continue;
		}

		Vector3 pullForce = Normalize( m_desiredVector );
		Vector3 forceOffset = pullForce * group.GetBoundingSphereRadius();
		forceVectors.push_back( drone->position + forceOffset );
		pullForce *= m_behaviorWeight * rampingForce;
		forceVectors.push_back( pullForce );
		drone->acceleration += pullForce;
	}
	return forceVectors;
}


void FollowASpline::RenderDebugInfo( Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	for ( auto tPoint = m_targetPointVector.begin(); tPoint != m_targetPointVector.end(); ++tPoint )
	{
		renderer.DrawSphere( this, TranslationMatrix( ( *tPoint ) ) * parentWorldLocation,
			2, 6, Tr2DebugRenderer::Wireframe, 0xff114444 );
	}
	return;
}