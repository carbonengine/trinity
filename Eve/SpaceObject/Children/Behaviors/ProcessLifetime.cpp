#include "StdAfx.h"
#include "ProcessLifetime.h"
#include "include/TriMath.h"

ProcessLifetime::ProcessLifetime( IRoot* lockobj ) :
	PARENTLOCK( m_splineTunnels ),
	m_firstAgentLifetime( 0 ), // Debug
	m_returningAge( -1 ),
	m_behaviorWeight( 900 ),
	m_wanderAmount( 0.3f ),
	m_shouldReassignTunnelIDs( true ),
	m_respawnAgentsOnDeath( true ),
	m_exit( false ),
	m_firstSpawnAtRandomPlaces( true ),
	m_intialSpawn( false ),
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

bool ProcessLifetime::Initialize()
{
	m_intialSpawn = m_firstSpawnAtRandomPlaces;

	return true;
}

void ProcessLifetime::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if( theList == &m_splineTunnels )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &ProcessLifetime::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		case BELIST_REMOVED:
			if( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &ProcessLifetime::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xff5555aa );
			}
			break;
		case BELIST_LOADFINISHED:
			if( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
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
	*static_cast<ProcessLifetimeData*>( scratchMemory ) = ProcessLifetimeData();
}

std::vector<Vector3> ProcessLifetime::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_shouldReassignTunnelIDs )
	{
		ReassignTunnelIDsAndAddSystemTunnels( system );
	}

	auto data = static_cast<ProcessLifetimeData*>( scratchData );

	int index = 0;
	std::vector<int> dronesThatDie;
	std::vector<Vector3> forceVectors;

	for( auto drone = agents.begin(); drone != agents.end(); ++drone, ++data )
	{
		if( drone->lifetime <= deltaTime && !m_intialSpawn )
		{
			FindASpawnPoint( *drone, data, group );
		}

		// find an initial spawn position
		if( !data->hasSpawned && m_intialSpawn )
		{
			Vector3 spawnPos = group.m_spawnPosition;
			SplineTunnelGroupVector* systemTunnels = system.GetSplineTunnels();
			if( FindInitialSpawnPoint( *drone, data, spawnPos, systemTunnels ) )
			{
				group.m_spawnPosition = spawnPos;
			}
			data->hasSpawned = true;
		}

		m_desiredVector = Vector3( 0, 0, 0 );

		if( !data->hasUsedEntryTunnel )
		{
			if( data->assignedLifeTimeTunnel == -1 )
			{
				data->hasUsedEntryTunnel = true;
			}
			else if( m_privateTunnels.size() < data->assignedLifeTimeTunnel )
			{
				data->hasUsedEntryTunnel = true;
			}
			else if( m_privateTunnels.size() > 0 )
			{
				if( data->assignedLifeTimeTunnel < m_privateTunnels.size() && ProcessTunnel( *drone, *m_privateTunnels[data->assignedLifeTimeTunnel], data->tunnelPoint, group.GetBoundingSphereRadius() ) )
				{
					data->tunnelPoint = 0;
					data->hasUsedEntryTunnel = true;
					data->assignedLifeTimeTunnel = -1;
				}
			}
		}

		if( m_exit || ( m_returningAge != -1 && drone->lifetime > m_returningAge ) )
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

					FindAndAssignAnExitTunnel( *drone, data );
				}
				else
				{
					if( m_privateTunnels.size() > 0 && data->assignedLifeTimeTunnel < m_privateTunnels.size() && ProcessTunnel( *drone, *m_privateTunnels[data->assignedLifeTimeTunnel], data->tunnelPoint, group.GetBoundingSphereRadius() ) )
					{
						data->hasUsedExitTunnel = true; //redundant add might refactor, keeping it atm
						dronesThatDie.push_back( index );
					}
				}
			}
		}
		index++;

		if( m_desiredVector == Vector3( 0, 0, 0 ) )
		{
			continue;
		}

		Vector3 pullForce = Normalize( m_desiredVector );
		Vector3 forceOffset = pullForce * group.GetBoundingSphereRadius();
		forceVectors.push_back( drone->position + forceOffset );
		pullForce *= m_behaviorWeight;
		forceVectors.push_back( pullForce );

		drone->acceleration += pullForce;
	}

	m_intialSpawn = false;

	for( int i = static_cast<int>( dronesThatDie.size() ) - 1; i >= 0; i-- )
	{
		group.RemoveSpecificAgent( dronesThatDie[i] );
		if( m_respawnAgentsOnDeath )
		{
			group.AddAgent();
		}
	}

	//debug
	if( !agents.empty() )
	{
		m_firstAgentLifetime = agents.begin()->lifetime;
	}

	return forceVectors;
}

float ProcessLifetime::GetRandomOffset( float cylWidth ) const
{
	return static_cast<float>( RAND_MAX / ( 2 * ( cylWidth * m_wanderAmount ) ) );
}

bool ProcessLifetime::ProcessTunnel( DroneAgent& agent, SplineTunnel& tunnel, int& pointID, float boundingSphere )
{
	if( tunnel.splinePoints.empty() )
	{
		return false;
	}

	// if we've reached the end of the tunnel
	if( pointID == tunnel.splinePoints.size() )
	{
		return true;
	}
	Vector3 targetVector = tunnel.splinePoints[pointID].pos - agent.position;
	Vector3 vectorBetween( 0, 0, 0 );

	if( tunnel.splinePoints[pointID] == ( *tunnel.splinePoints.begin() ) )
	{
		vectorBetween = tunnel.splinePoints[pointID].rot;
	}
	else
	{
		vectorBetween = tunnel.splinePoints[pointID - 1].rot;
	}

	const float lengthBetweenPoints = Length( vectorBetween );


	if( 0 != lengthBetweenPoints )
	{
		const float dotProd = Dot( targetVector, vectorBetween );
		const Vector3 vectorProj = ( dotProd / ( lengthBetweenPoints * lengthBetweenPoints ) ) * vectorBetween;
		Vector3 offset = ( tunnel.cylWidth / 2 ) * Normalize( vectorProj - targetVector );

		// add a random offset so drones will kind of wander around the tunnel seeking the next point
		for( int i = 0; i < 3; i++ )
		{
			offset[i] += -( tunnel.cylWidth * m_wanderAmount ) + static_cast<float>( rand() ) / GetRandomOffset( tunnel.cylWidth );
		}

		targetVector += offset;
	}

	if( tunnel.splinePoints[pointID] == ( *tunnel.splinePoints.end() ) )
	{
		m_desiredVector = tunnel.splinePoints[pointID].rot;

		// the Dot product is positive if the agent is facing the target point
		if( Dot( targetVector, Vector3( agent.rotation ) ) < 0 || Length( targetVector ) > 2 * lengthBetweenPoints )
		{
			return true;
		}
	}
	else
	{
		const float lengthFromShip = Length( targetVector );

		float blendingMod = 0;

		if( lengthBetweenPoints != 0 )
		{
			blendingMod = min( 1.f, max( 0.f, ( lengthBetweenPoints - lengthFromShip ) / lengthBetweenPoints ) );
			blendingMod = blendingMod * blendingMod;
		}

		m_desiredVector = 0.8f * ( 1 - blendingMod ) * Normalize( targetVector ) +
			( 1 - 0.8f ) * blendingMod * Normalize( tunnel.splinePoints[pointID].rot + targetVector );

		if( Dot( Normalize( targetVector ), Normalize( m_desiredVector ) ) < 0.8f )
		{
			m_desiredVector = targetVector;
		}

		if( ( lengthFromShip - boundingSphere ) < ( tunnel.cylWidth ) / 1.5 )
		{
			pointID++;
		}
	}

	return false;
}

void ProcessLifetime::FindAndAssignAnExitTunnel( const DroneAgent& agent, ProcessLifetimeData* data )
{
	int closestPointIndex = -1;
	float lengthSqToClosestPoint = -1;
	int index = 0;
	for( auto tunnel = begin( m_privateTunnels ); tunnel != end( m_privateTunnels ); ++tunnel )
	{
		if( ( *tunnel )->tunnelGroupType == EXIT_TUNNELS )
		{
			Vector3 vectorToLocation = ( *tunnel )->splinePoints[0].pos - agent.position;
			if( lengthSqToClosestPoint == -1 || LengthSq( vectorToLocation ) < lengthSqToClosestPoint )
			{
				lengthSqToClosestPoint = LengthSq( vectorToLocation );
				closestPointIndex = index;
			}
		}
		index++;
	}
	if( closestPointIndex != -1 )
	{
		data->assignedLifeTimeTunnel = closestPointIndex;
	}
	else
	{
		data->hasUsedExitTunnel = true;
	}
}

bool ProcessLifetime::FindInitialSpawnPoint( DroneAgent& drone, ProcessLifetimeData* data, Vector3& pos, SplineTunnelGroupVector* systemTunnels )
{
	// if we have local tunnels use them, otherwise use system wide ones
	SplineTunnelGroupVector* tunnels;
	if( !m_splineTunnels.empty() )
	{
		tunnels = &m_splineTunnels;
	}
	else if( !systemTunnels->empty() )
	{
		tunnels = systemTunnels;
	}
	else
	{
		return false;
	}

	size_t sizeIndex = tunnels->size();

	// random nr from 0 to sizeIndex
	size_t randomNr = rand() % ( sizeIndex );

	// pick a random splineTunnel
	auto splineTunnel = (*tunnels)[randomNr];

	// return early if there are no curves or the curves aren't loaded
	if( splineTunnel->GetCurveSets()->empty() )
	{
		return false;
	}

	if( splineTunnel->GetTunnelGroupType() == SplineTunnelGroup::ENTRANCE_TUNNELS )
	{
		// get the curve sets for that tunnel
		auto curveSets = splineTunnel->GetCurveSets();

		size_t curveSize = ( *curveSets ).size();
		// we can have more than 1 curve so pick a random curve
		size_t randomCurve = rand() % ( curveSize );

		// get random time
		size_t length = size_t( ( *curveSets )[randomCurve]->Length() );
		size_t time = rand() % ( length + 1 );

		// get value at time
		pos = ( *curveSets )[randomCurve]->GetValue( double( time ) );

		float stepSize = float( time ) / float( length );

		// Get the next pointID
		auto pointID = floor( stepSize * ( splineTunnel->GetNumBreakPoints() ) + 1 ) + 0.5f;

		drone.position = pos;
		drone.lifetime += stepSize * pointID;
		data->tunnelPoint = static_cast<int>( pointID );

		return true;
	}
	return false;
}

void ProcessLifetime::FindASpawnPoint( DroneAgent& agent, ProcessLifetimeData* data, BehaviorGroup& group )
{
	std::vector<Vector3> potentialPoints;
	std::vector<Vector3> potentialRotations;
	std::vector<int> tunnelIndex;

	for( auto tunnel = begin( m_privateTunnels ); tunnel != end( m_privateTunnels ); ++tunnel )
	{
		if( ( *tunnel )->tunnelGroupType == TunnelGroupType::ENTRANCE_TUNNELS )
		{
			if( !( *tunnel )->splinePoints.empty() )
			{
				Vector3 point = ( *tunnel )->splinePoints[0].pos;
				for( int i = 0; i < 3; i++ )
				{
					point[i] += -( *tunnel )->pointOfNoReturnSize + static_cast<float>( rand() ) / ( static_cast<float>( RAND_MAX / ( 2 * ( *tunnel )->pointOfNoReturnSize ) ) );
				}
				potentialPoints.push_back( point );
				potentialRotations.push_back( ( *tunnel )->splinePoints[0].rot );
				tunnelIndex.push_back( ( *tunnel )->tunnelID );
			}
		}
	}	

	if( potentialPoints.empty() )
	{
		return;
	}

	const auto randomNbr = rand() % potentialPoints.size();
	group.m_spawnPosition = potentialPoints.at( randomNbr );
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
		if( ( *tunnel )->tunnelGroupType == TunnelGroupType::ENTRANCE_TUNNELS )
		{
			if( !( *tunnel )->splinePoints.empty() )
			{
				Vector3 point = ( *tunnel )->splinePoints[0].pos;
				entrancePoints.push_back( point );
			}
		}
	}
	return entrancePoints;
}

void ProcessLifetime::UpdateTunnelRegistry()
{
	m_privateTunnels.clear();
	if( !m_splineTunnels.empty() )
	{
		for( auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it )
		{
			const auto tunnelGroup = ( *it )->GetTunnels();

			for( auto tunnel = begin( *tunnelGroup ); tunnel != end( *tunnelGroup ); ++tunnel )
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

	for( auto it = begin( *tunnels ); it != end( *tunnels ); ++it )
	{
		// puts a de-const-ified version of the system-tunnel-pointer at the front of the vector
		m_privateTunnels.insert( m_privateTunnels.begin(), const_cast<SplineTunnel*>( &( *it ) ) );
	}

	int id = 0;

	if( !tunnels->empty() )
	{
		id = tunnels->back().tunnelID;
	}

	if( m_privateTunnels.empty() )
	{
		m_shouldReassignTunnelIDs = false;
		return;
	}

	for( auto it = begin( m_privateTunnels ); it != end( m_privateTunnels ); ++it )
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
	if( renderer.HasOption( this, "SplineTunnels" ) )
	{
		for( auto t = m_splineTunnels.begin(); t != m_splineTunnels.end(); ++t )
		{
			( *t )->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}
}
