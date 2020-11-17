#include "StdAfx.h"
#include "ProcessLifetime.h"
#include "include/TriMath.h"

ProcessLifetime::ProcessLifetime( IRoot* lockobj ) :
	PARENTLOCK( m_splineTunnels ),
	m_firstAgentLifetime( 0 ), // Debug
	m_returningAge( -1 ),
	m_behaviorWeight( 15 ),
	m_shouldReassignTunnelIDs( true ),
	m_respawnAgentsOnDeath( true ),
	m_exit( false ),
	m_priority( LEAST_PRIORITY )
{
	m_splineTunnels.SetNotify( this );
}

ProcessLifetime::~ProcessLifetime()
{
}

bool ProcessLifetime::OnModified( Be::Var* value )
{
	UpdateTunnelRegistry();
	return true;
}

void ProcessLifetime::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if ( theList == &m_splineTunnels )
	{
		switch ( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if ( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &ProcessLifetime::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		case BELIST_REMOVED:
			if ( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &ProcessLifetime::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		case BELIST_LOADFINISHED:
			if ( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &ProcessLifetime::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		default:
			break;
		}
		UpdateTunnelRegistry();
	}
}

int ProcessLifetime::GetProcessPriority()
{
	return m_priority;
}

std::string ProcessLifetime::GetBehaviorName()
{
	return "ProcessLifetime";
}

size_t ProcessLifetime::GetScratchMemorySize() const
{
	return sizeof( ProcessLifetimeData );
}

void ProcessLifetime::InitializeScratch( void* scratchMemory )
{
	*static_cast< ProcessLifetimeData* >( scratchMemory ) = ProcessLifetimeData();
}

std::vector<Vector3> ProcessLifetime::CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
                                                        BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	if ( m_shouldReassignTunnelIDs )
	{
		ReassignTunnelIDsAndAddSystemTunnels( system );
	}

	auto data = static_cast< ProcessLifetimeData* >( scratchData );

	int index = 0;
	std::vector<int> dronesThatDie;
	std::vector<Vector3> forceVectors;

	for ( auto drone = agents.begin(); drone != agents.end(); ++drone, data++ )
	{
		if( drone->lifetime <= deltaTime )
		{
			FindASpawnPoint( *drone, data );
		}

		m_desiredVector = Vector3( 0, 0, 0 );

		if( !data->hasUsedEntryTunnel )
		{
			if( data->assignedLifeTimeTunnel == -1)
			{
				data->hasUsedEntryTunnel = true;
			}
			else if( m_privateTunnels.size() < data->assignedLifeTimeTunnel )
			{
				data->hasUsedEntryTunnel = true;
			}
			else
			{
				if( ProcessTunnel( *drone, *m_privateTunnels[ data->assignedLifeTimeTunnel ], data->tunnelPoint, group.GetBoundingSphereRadius() ) )
				{
					data->tunnelPoint = 0;
					data->hasUsedEntryTunnel = true;
					data->assignedLifeTimeTunnel = -1;
				}
			}
		}

		if( m_exit || (m_returningAge != -1 && drone->lifetime > m_returningAge) )
		{
			// If drone has exited then remove it
			if( data->hasUsedExitTunnel )
			{
				dronesThatDie.push_back( index );
			}
			else
			{
				if( data->assignedLifeTimeTunnel == -1 )
				{
					if( data->hasUsedExitTunnel )
					{
						dronesThatDie.push_back( index );
					}

					findAndAssignAnExitTunnel( *drone, data );
				}
				else
				{
					if ( ProcessTunnel( *drone, *m_privateTunnels[ data->assignedLifeTimeTunnel ], data->tunnelPoint, group.GetBoundingSphereRadius() ) )
					{
						data->hasUsedExitTunnel = true; //redundant add might refactor, keeping it atm
						dronesThatDie.push_back( index );
					}
				}
			}
		}
		index++;

		if ( m_desiredVector == Vector3( 0, 0, 0 ) )
		{
			//m_lastPullForces.push_back( Vector3( 0, 0, 0 ) );
			continue;
		}

		Vector3 pullForce = Normalize( m_desiredVector );
		Vector3 forceOffset = pullForce * group.GetBoundingSphereRadius();
		forceVectors.push_back( drone->position + forceOffset );
		pullForce *= m_behaviorWeight;
		forceVectors.push_back( pullForce );
		drone->acceleration += pullForce;
	}

	for( int i = static_cast<int>(dronesThatDie.size()) - 1; i >= 0; i-- )
	{
		group.RemoveSpecificAgent( dronesThatDie[i] );
		if( m_respawnAgentsOnDeath )
		{
			group.AddAgent();
		}
	}

	//debug
	if ( !agents.empty() )
	{
		m_firstAgentLifetime = agents.begin()->lifetime;
	}

	return forceVectors;
}

bool ProcessLifetime::ProcessTunnel( DroneAgent& agent, SplineTunnel& tunnel, int& pointID, float boundingSphere )
{
	Vector3 targetVector = tunnel.splinePoints[ pointID ].pos - agent.position;
	Vector3 vectorBetween( 0, 0, 0 );

	if ( tunnel.splinePoints[ pointID ] == ( *tunnel.splinePoints.begin() ) )
	{
		vectorBetween = tunnel.splinePoints[ pointID ].rot;
	}
	else
	{
		vectorBetween = tunnel.splinePoints[ pointID - 1 ].rot;
	}

	const float lengthBetweenPoints = Length( vectorBetween );


	if ( 0 != lengthBetweenPoints )
	{
		// an offset is added to the target point so that they don't all follow the same line
		const float dotProd = Dot( targetVector, vectorBetween );
		const Vector3 vectorProj = ( dotProd / ( lengthBetweenPoints * lengthBetweenPoints ) ) * vectorBetween;
		const Vector3 offset = ( tunnel.cylWidth / 2 ) * Normalize( vectorProj - targetVector );
		targetVector += offset;
	}

	Vector3 desiredVector( 0, 0, 0 );

	if ( tunnel.splinePoints[ pointID ] == ( *tunnel.splinePoints.end() ) )
	{
		m_desiredVector = tunnel.splinePoints[ pointID ].rot;

		// the Dot product is positive if the agent is facing the target point
		if ( Dot( targetVector, Vector3( agent.rotation ) ) < 0  || Length( targetVector ) > 2 * lengthBetweenPoints )
		{
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

		m_desiredVector = 0.8f * ( 1 - blendingMod ) * Normalize( targetVector ) +
			( 1 - 0.8f ) * blendingMod * Normalize( tunnel.splinePoints[ pointID ].rot + targetVector );

		if ( Dot( Normalize( targetVector ), Normalize( m_desiredVector ) ) < 0.8f )
		{
			m_desiredVector = targetVector;
		}

		if ( ( lengthFromShip - boundingSphere ) < ( tunnel.cylWidth ) / 1.5 )
		{
			pointID++;
		}
	}

	return false;
}

void ProcessLifetime::findAndAssignAnExitTunnel( DroneAgent& agent, ProcessLifetimeData* data )
{
	//Vector3 closestPoint;
	int closestPointIndex = -1;
	float lengthSqToClosestPoint = -1;
	int index = 0;
	for ( auto tunnel = begin( m_privateTunnels ); tunnel != end( m_privateTunnels ); ++tunnel )
	{
		if ( ( *tunnel )->tunnelGroupType == EXIT_TUNNELS )
		{
			Vector3 vectorToLocation = ( *tunnel )->splinePoints[ 0 ].pos - agent.position;
			if( lengthSqToClosestPoint == -1 || LengthSq(vectorToLocation) < lengthSqToClosestPoint )
			{
				lengthSqToClosestPoint = LengthSq( vectorToLocation );
				closestPointIndex = index;
			}
		}
		index++;
	}
	if( closestPointIndex != -1)
	{
		data->assignedLifeTimeTunnel = closestPointIndex;
	}
	else
	{
		data->hasUsedExitTunnel = true;
	}
}

void ProcessLifetime::FindASpawnPoint(DroneAgent& agent, ProcessLifetimeData* data )
{
	std::vector<Vector3> potentialPoints;
	std::vector<Vector3> potentialRotations;
	std::vector<int> tunnelIndex;

	for ( auto tunnel = begin( m_privateTunnels ); tunnel != end( m_privateTunnels ); ++tunnel )
	{
		if( ( *tunnel )->tunnelGroupType == ENTRANCE_TUNNELS )
		{	
			Vector3 point = (*tunnel)->splinePoints[ 0 ].pos;
			for(int i = 0; i < 3; i++)
			{
				point[i] += -(*tunnel)->pointOfNoReturnSize + static_cast < float > ( rand() ) /
					( static_cast < float > ( RAND_MAX / ( 2 * (*tunnel)->pointOfNoReturnSize ) ) );
			}
			potentialPoints.push_back( point );
			potentialRotations.push_back( (*tunnel)->splinePoints[ 0 ].rot );
			tunnelIndex.push_back( ( *tunnel )->tunnelID );
		}
	}

	if( potentialPoints.empty() )
	{
		agent.position = Vector3( 0, 0, 0 );
		return;
	}

	const auto randomNbr = rand() % potentialPoints.size();
	agent.position = potentialPoints.at( randomNbr );
	static const Vector3 zAxis( 0.f, 0.f, 1.f );
	TriQuaternionRotationArc( &agent.rotation, &zAxis, &potentialRotations.at( randomNbr ) );
	data->assignedLifeTimeTunnel = tunnelIndex.at( randomNbr );
}

std::vector<Vector3> ProcessLifetime::GetEntrancePoints()
{
	std::vector<SplineTunnel*> tempVec;
	std::vector<Vector3> entrancePoints;
	for( auto tunnel = begin( m_privateTunnels ); tunnel != end( m_privateTunnels ); ++tunnel )
	{
		if( ( *tunnel )->tunnelGroupType == ENTRANCE_TUNNELS )
		{
			Vector3 point = ( *tunnel )->splinePoints[0].pos;
			entrancePoints.push_back( point );
		}
	}
	return entrancePoints;
}	

void ProcessLifetime::UpdateTunnelRegistry()
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

void ProcessLifetime::ReassignTunnelIDsAndAddSystemTunnels( EveChildBehaviorSystem& system )
{
	const auto tunnels = system.GetTunnels();

	for ( auto it = begin( *tunnels ); it != end( *tunnels ); ++it )
	{
		// puts a de-const-ified version of the system-tunnel-pointer at the front of the vector 
		m_privateTunnels.insert( m_privateTunnels.begin(), const_cast< SplineTunnel* > ( &( *it ) ) );
	}

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
		( *it )->tunnelID = id;
		id++;
	}

	m_shouldReassignTunnelIDs = false;
}

void ProcessLifetime::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "SplineTunnels" );
}

void ProcessLifetime::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if ( renderer.HasOption( this, "SplineTunnels" ) )
	{
		for ( auto t = m_splineTunnels.begin(); t != m_splineTunnels.end(); ++t )
		{
			( *t )->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}
}
