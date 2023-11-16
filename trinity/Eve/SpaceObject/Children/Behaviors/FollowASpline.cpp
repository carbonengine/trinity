#include "StdAfx.h"
#include "followASpline.h"
#include "Formation.h"

FollowASpline::FollowASpline( IRoot* lockobj ) :
	PARENTLOCK( m_splineTunnels ),
	m_enabled( true ),
	m_shouldReassignTunnelIDs( true ),
	m_behaviorWeight( 600.f ),
	m_smoothPullFactor( 0.8 ),
	m_cornerSmoothener( 0.8 ),
	m_tunnelGroupType( OTHER_TUNNELS ),
	m_priority( LEAST_PRIORITY ),
	m_frameCounter( 0 ),
	m_framesBetweenUpdates( 11 )
{
	m_splineTunnels.SetNotify( this );
}

FollowASpline::~FollowASpline()
{
}

bool FollowASpline::OnModified( Be::Var* value )
{
	UpdateTunnelRegistry();
	return true;
}

int FollowASpline::GetProcessPriority()
{
	return m_priority;
}

void FollowASpline::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if ( theList == &m_splineTunnels )
	{
		switch ( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if ( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &FollowASpline::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		case BELIST_REMOVED:
			if ( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &FollowASpline::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		case BELIST_LOADFINISHED:
			if ( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &FollowASpline::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		default:
			break;
		}
	}
}

float FollowASpline::ProcessTunnelEntrances( DroneAgent& agent, const std::vector<SplineTunnel*>& tunnels, FollowASplineData* data )
{
	// not associated with a tunnel
	for ( auto tunnel = tunnels.begin(); tunnel != tunnels.end(); ++tunnel )
	{
		auto t = ( *tunnel );

		if( t->tunnelGroupType != OTHER_TUNNELS )
		{
			return 0;
		}

		Vector3 dist = t->splinePoints[ 0 ].pos - agent.position;
		float length = Length( dist );
		if ( length < t->pointOfNoReturnSize )
		{
			data->tunnelLock = t->tunnelID;
			data->tunnelPoint = 0;
		}
		else if ( length < t->pullSize )
		{
			if ( t->pullSize == t->pointOfNoReturnSize )
			{
				continue;
			}

			// normalize the distance between outer and inner spheres to increase pull-strength
			float mod = ( length - t->pointOfNoReturnSize ) / ( t->pullSize - t->pointOfNoReturnSize );
			mod = 1 - max( 0.f, min( mod, 1.f ) );
			m_desiredVector = dist;
			return min( 1.f, max( 0.f, 1 - m_smoothPullFactor + ( m_smoothPullFactor * mod ) ) );
		}
	}
	return 1.f;
}


bool FollowASpline::ProcessAssignedTunnel(DroneAgent& agent, const std::vector<SplineTunnel*>& tunnels,
                                          BehaviorGroup& group, FollowASplineData* data)
{
	if( data->tunnelLock > static_cast< int >(tunnels.size()))
	{
		return false;
	}

	auto t = tunnels.at( data->tunnelLock );
	const auto pID = data->tunnelPoint; // Point ID

	Vector3 targetVector = t->splinePoints[ pID ].pos - agent.position;
	Vector3 vectorBetween( 0, 0, 0 );

	if ( t->splinePoints[ pID ] == ( *t->splinePoints.begin() ) )
	{
		vectorBetween = t->splinePoints[ pID ].rot;
	}
	else
	{
		vectorBetween = t->splinePoints[ pID - 1 ].rot;
	}

	const float lengthBetweenPoints = Length(vectorBetween);

	if( 0 != lengthBetweenPoints )
	{	
		// an offset is added to the target point so that they don't all follow the same line
		const float dotProd = Dot( targetVector, vectorBetween );
		const Vector3 vectorProj = ( dotProd / ( lengthBetweenPoints * lengthBetweenPoints ) ) * vectorBetween;
		const Vector3 offset = ( t->cylWidth / 2 ) * Normalize( vectorProj - targetVector );
		targetVector += offset;
	}

	m_targetPointVector.push_back( targetVector + agent.position );
	

	if ( t->splinePoints[ pID ] == ( *t->splinePoints.end() ) )
	{
		m_desiredVector = t->splinePoints[ pID ].rot;

		// the Dot product is positive if the agent is facing the target point
		if ( Dot( targetVector, Vector3( agent.rotation ) ) < 0 )
		{
			data->tunnelLock = -1;
			data->tunnelPoint = 0;
			return true;
		}
	}
	else
	{
		const float lengthFromShip = Length( targetVector );

		float blendingMod = 0;

		if ( lengthBetweenPoints != 0 )
		{
			blendingMod = min( 1.f, max( 0.f, ( lengthBetweenPoints - lengthFromShip ) / lengthBetweenPoints ) );
			blendingMod = blendingMod * blendingMod;
		}
		m_cornerSmoothener = min( 1.f, max( 0.f, m_cornerSmoothener ) );
		m_desiredVector = m_cornerSmoothener * ( 1 - blendingMod ) * Normalize( targetVector ) + 
							( 1 - m_cornerSmoothener ) * blendingMod * Normalize( t->splinePoints[ pID ].rot + targetVector );
		if ( Dot( Normalize( targetVector ), Normalize( m_desiredVector ) ) < m_cornerSmoothener )
		{
			m_desiredVector = targetVector;
		}

		if ((lengthFromShip - group.GetBoundingSphereRadius()) < (t->cylWidth)/1.5)
		{
			data->tunnelPoint++;
			return true;
		}

		//rework into cylinder collision
		if ( lengthFromShip > ( group.GetBoundingSphereRadius() + lengthBetweenPoints * 1.5) && Dot( targetVector, vectorBetween ) < 0 )
		{
			data->tunnelLock = -1;
			data->tunnelPoint = 0;
			return true;
		}
	}
	return false;
}

size_t FollowASpline::GetScratchMemorySize() const
{
	return sizeof( FollowASplineData );
}

void FollowASpline::InitializeScratch( void* scratchMemory )
{
	*static_cast<FollowASplineData*>( scratchMemory ) = FollowASplineData();
}

std::vector<Vector3> FollowASpline::CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
                                                      BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> forceVectors;
	if( !m_enabled )
	{
		return forceVectors;
	}

	if ( m_frameCounter >= m_framesBetweenUpdates )
	{
		m_frameCounter = 0;
	}
	else
	{
		m_frameCounter++;
	}


	if ( m_frameCounter == 0 )
	{
		if( m_shouldReassignTunnelIDs )
		{
			ReassignTunnelIDsAndAddSystemTunnels( system );
			group.InitializeGeometryResource(); // reset all agents
			return forceVectors;
		}

		m_lastPullForces.clear();
		m_targetPointVector.clear();

		auto data = static_cast< FollowASplineData* >( scratchData );
		for ( auto drone = agents.begin(); drone != agents.end(); ++drone, data++ )
		{
			m_desiredVector = Vector3( 0, 0, 0 );
			float rampingForce = 1;

			if ( data->tunnelLock == -1 )
			{
				rampingForce = ProcessTunnelEntrances( *drone, m_privateTunnels, data );
			}

			// tunnelLock can change in ProcessTunnelEntrances so if->else is not equivalent
			if ( data->tunnelLock != -1 )
			{
				if( ProcessAssignedTunnel( *drone, m_privateTunnels, group, data ) )
				{
					// If process returns true we update all the drones as a unit and skip if they are in a Formation
					if( CheckForAndUpdateFormation( agents, group, scratchData, data->tunnelLock, data->tunnelPoint ) )
					{
						// If process returns true all drones have been updated so we break
						break;
					}
				}
			}

			if ( m_desiredVector == Vector3( 0, 0, 0 ) )
			{
				m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
				continue;
			}

			Vector3 pullForce = Normalize( m_desiredVector );
			Vector3 forceOffset = pullForce * group.GetBoundingSphereRadius();
			forceVectors.push_back( drone->position + forceOffset );
			pullForce *= m_behaviorWeight * rampingForce;
			forceVectors.push_back( pullForce );
			drone->acceleration += pullForce;
			m_lastPullForces.push_back( pullForce );
		}
	}
	else
	{
		if ( m_lastPullForces.empty() )
		{
			return forceVectors;
		}
		int c = 0;
		for ( auto agent = agents.begin(); agent != agents.end(); ++agent, c++ )
		{
			agent->acceleration += m_lastPullForces[ c ];

			if ( group.m_collectForces && m_lastPullForces[ c ] != Vector3( 0, 0, 0 ) )
			{
				Vector3 forceOffset = Normalize( m_lastPullForces[ c ] ) * group.GetBoundingSphereRadius();
				forceVectors.push_back( agent->position + forceOffset );
				forceVectors.push_back( m_lastPullForces[ c ] );
			}
		}
	}
	return forceVectors;
}

bool FollowASpline::CheckForAndUpdateFormation( std::vector<DroneAgent>& agents, BehaviorGroup& group, void* scratchData, int tunnel, int tunnelPoint )
{
	IBehavior* formationBH = group.GetBehaviorByName( "Formation" );

	if( formationBH )
	{
		auto form = dynamic_cast<Formation*> ( formationBH );
		if( form )
		{
			if( form->InFormation() )
			{
				auto dataPointer = static_cast<FollowASplineData*>( scratchData );
				for( auto agent = agents.begin(); agent != agents.end(); ++agent, dataPointer++ )
				{
					dataPointer->tunnelLock = tunnel;
					dataPointer->tunnelPoint = tunnelPoint;
				}
				return true;
			}
		}
	}
	return false;
}

void FollowASpline::UpdateTunnelRegistry()
{
	m_privateTunnels.clear();
	if ( !m_splineTunnels.empty() )
	{
		for ( auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it )
		{
			const auto tunnelGroup = ( *it )->GetTunnels();

			for ( auto tunnel = begin( *tunnelGroup ); tunnel != end( *tunnelGroup ); ++tunnel )
			{
				m_privateTunnels.push_back( &*tunnel );
			}
		}
	}
	m_shouldReassignTunnelIDs = true;
}

void FollowASpline::ReassignTunnelIDsAndAddSystemTunnels( EveChildBehaviorSystem& system )
{

	const auto tunnels = system.GetTunnels();

	//std::vector<SplineTunnel*> pointersToTunnels;
	for ( auto it = begin( *tunnels ); it != end( *tunnels ); ++it )
	{
		m_privateTunnels.insert( m_privateTunnels.begin(), const_cast< SplineTunnel* > (&(*it)) );
	}

	//m_privateTunnels.insert( m_privateTunnels.begin(), tunnels->begin(), tunnels->end() );

	int id = 0;

	if ( !tunnels->empty() )
	{
		id = tunnels->back().tunnelID;
	}	

	if ( m_privateTunnels.empty() )
	{
		m_shouldReassignTunnelIDs = false;
		return;
	}

	for ( auto it = begin( m_privateTunnels ); it != end( m_privateTunnels ); ++it )
	{
		(*it)->tunnelID = id;
		id++;
	}

	m_shouldReassignTunnelIDs = false;
}

void FollowASpline::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "SplineTunnels" );
}

void FollowASpline::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if ( renderer.HasOption( this, "SplineTunnels" ) )
	{
		for ( auto tPoint = m_targetPointVector.begin(); tPoint != m_targetPointVector.end(); ++tPoint )
		{
			renderer.DrawSphere( this, TranslationMatrix( ( *tPoint ) ) * parentWorldLocation,
				2, 6, Tr2DebugRenderer::Wireframe, 0xff114444 );
		}

		for ( auto t = m_splineTunnels.begin(); t != m_splineTunnels.end(); ++t )
		{
			( *t )->RenderDebugInfo( renderer, parentWorldLocation );
		}
		return;
	}
}