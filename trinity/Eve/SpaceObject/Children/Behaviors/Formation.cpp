#include "StdAfx.h"
#include "Formation.h"
#include "Include/TriMath.h"


Formation::Formation( IRoot* lockobj ) :
	m_enabled( true ),
	m_formationSpeed( 0, 0, 0 ),
	m_formationRotation( 0, 0, 0 ),
	m_formationPosition( 0, 0, 0 ),
	m_lastFormationAcceleration( 0, 0, 0 ),
	m_maxFormationVelocityScaler( 0.85 ),
	m_behaviorWeight( 300 ),
	m_stubbornness( 3 ),
	m_stubbornnessCounter( 0 ),
	m_visionRange( 5 ),
	m_frameCounter( 0 ),
	m_maxFormationRotationSpeed( 0.314 ),
	m_framesBetweenUpdates( 15 ),
	m_inFormation( false ),
	m_isFormalizing( false ),
	m_priority( LEAST_PRIORITY )
{
	m_lastPullForces.clear();
}

Formation::~Formation()
{
}

size_t Formation::GetScratchMemorySize() const
{
	return sizeof( FormationData );
}

void Formation::InitializeScratch( void* scratchMemory )
{
	*static_cast<FormationData*>( scratchMemory ) = FormationData();
}

int Formation::GetProcessPriority()
{
	return m_priority;
}

std::string Formation::GetBehaviorName()
{
	return "Formation";
}

bool Formation::InFormation()
{
	return m_inFormation;
}

std::vector<Vector3> Formation::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> returnForces;
	if( !m_enabled )
	{
		return returnForces;
	}

	if( m_frameCounter >= m_framesBetweenUpdates )
	{
		//we'll do a check
		if( CheckIfFormalizing( agents, scratchData ) )
		{
			// we only enable formation if the check is passed couple of times in a row (consistency)
			if( !m_inFormation && m_stubbornnessCounter >= m_stubbornness )
			{
				InitializeFormation( agents, scratchData, group.GetBoundingSphereRadius() );
			}
			else
			{
				if( m_isFormalizing == true )
				{
					m_stubbornnessCounter++;
				}
				else
				{
					m_stubbornnessCounter = 0;
				}

				m_isFormalizing = true;
			}
		}
		else
		{
			if( m_inFormation && m_stubbornnessCounter >= m_stubbornness )
			{
				BreakFormation();
			}
			else
			{
				if( m_isFormalizing == true )
				{
					m_stubbornnessCounter++;
				}
				else
				{
					m_stubbornnessCounter = 0;
				}

				m_isFormalizing = false;
			}
		}
	}
	else
	{
		m_frameCounter++;
	}

	if( m_inFormation )
	{
		UpdateFormation( deltaTime, group );
		UpdateAgents( agents, scratchData, group.GetBoundingSphereRadius() );
	}

	return returnForces;
}

bool Formation::CheckIfFormalizing( std::vector<DroneAgent>& agents, void* scratchData )
{
	if( agents.empty() )
	{
		return false;
	}


	// test if agents are being dragged in the same direction (only accounting for behaviors before this one)
	int disagreeingDrones = 0;
	Vector3 groupAllignmentDirection = Normalize( agents.begin()->acceleration );
	auto data = static_cast<FormationData*>( scratchData );
	m_formationAcceleration = Vector3( 0.f, 0.f, 0.f );

	for( auto agent = agents.begin(); agent != agents.end(); ++agent, data++ )
	{
		Vector3 accelNormalized = Normalize( agent->acceleration );

		if( !( Dot( accelNormalized, groupAllignmentDirection ) > 0 ) )
		{
			if( m_inFormation )
			{
				if( Dot( accelNormalized, Normalize( m_formationPosition + m_formationGrid[data->assignedSlot] - agent->position ) ) < 0 )
				{
					disagreeingDrones++;
				}
			}
			else
			{
				disagreeingDrones++;
				groupAllignmentDirection = Lerp( groupAllignmentDirection, accelNormalized, 0.5 );
			}
		}

		m_formationAcceleration += accelNormalized;
	}

	if( disagreeingDrones >= 0.1 * float( agents.size() ) )
	{
		return false;
	}

	return true;
}

void Formation::InitializeFormation( std::vector<DroneAgent>& agents, void* scratchData, float radius )
{
	if( agents.empty() )
	{
		return;
	}

	//find new centerPoint
	m_formationPosition = Vector3( 0.f, 0.f, 0.f );
	Vector3 targetDir( 0.f, 0.f, 0.f );

	for( auto agent = agents.begin(); agent != agents.end(); ++agent )
	{
		m_formationPosition += agent->position;
		targetDir += agent->acceleration;
	}

	m_formationPosition = m_formationPosition / float( agents.size() );

	CreateFormationGrid( agents, targetDir, radius );
	AssignSlots( agents, scratchData );

	m_inFormation = true;
}

void Formation::CreateFormationGrid( std::vector<DroneAgent>& agents, Vector3& targetDir, float radius )
{

	targetDir.y = m_formationPosition.y;
	targetDir = Normalize( targetDir );

	auto c = Vector3( 0, 1, 0 );
	auto angle = AngleFromNormalized( c, targetDir );
	auto quat = RotationQuaternion( c, angle );

	m_formationGrid.clear();
	m_formationGridReserver.clear();

	float num = float( agents.size() );
	num = floor( sqrt( num - 1.f ) ) + 1.f;

	for( int i = 0; i < num; i++ )
	{
		for( int j = 0; j < num; j++ )
		{
			float r = 2.5f * radius;
			Vector3 temp( ( -( num / 2.f ) + i ) * r, 0.f, ( -( num / 2.f ) + j ) * r );

			TriVectorRotateQuaternion( &temp, &temp, &quat );
			m_formationGrid.push_back( temp );
			m_formationGridReserver.push_back( false );
		}
	}
}

void Formation::AssignSlots( std::vector<DroneAgent>& agents, void* scratchData )
{
	auto data = static_cast<FormationData*>( scratchData );
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, data++ )
	{
		int nearestSlot = -1;
		float closestLngth = 0;
		int index = 0;
		for( auto slot = m_formationGrid.begin(); slot != m_formationGrid.end(); ++slot, index++ )
		{
			if( !m_formationGridReserver[index] )
			{
				if( nearestSlot == -1 )
				{
					nearestSlot = index;
					closestLngth = LengthSq( agent->position - ( m_formationPosition + *slot ) );
				}
				else
				{
					float lengthToSlot = LengthSq( agent->position - ( m_formationPosition + *slot ) );
					if( lengthToSlot < closestLngth )
					{
						nearestSlot = index;
						closestLngth = lengthToSlot;
					}
				}
			}
		}
		m_formationGridReserver[nearestSlot] = true;
		data->assignedSlot = nearestSlot;
	}
}


void Formation::Reset()
{
	BreakFormation();
}

void Formation::BreakFormation()
{
	m_formationPosition = Vector3( 0.f, 0.f, 0.f );
	m_formationSpeed = Vector3( 0.f, 0.f, 0.f );
	m_formationRotation = Vector3( 0.f, 0.f, 0.f );
	m_lastFormationAcceleration = Vector3( 0.f, 0.f, 0.f );
	m_formationAcceleration = Vector3( 0.f, 0.f, 0.f );
	m_isFormalizing = false;
	m_inFormation = false;
	m_formationGrid.clear();
	m_formationGridReserver.clear();
}

void Formation::UpdateFormation( const float deltaTime, BehaviorGroup& group )
{
	// update the initialGrid like a single ship based on the closest drone
	//DroneAgent* agent = group.GetKDTree()->FindClosestAgent( m_formationPosition );

	Vector3 acceleration = m_formationAcceleration;

	//todo: use avg accel instead?

	calculateFormationInertia( acceleration, deltaTime );

	m_formationSpeed += acceleration;
	m_formationSpeed = ClampLength( m_formationSpeed, group.GetMaxVelocity() * m_maxFormationVelocityScaler );

	m_formationPosition += m_formationSpeed * deltaTime;
}

void Formation::calculateFormationInertia( Vector3& acceleration, const float deltaTime )
{
	auto accelNormalized = Normalize( acceleration );

	if( LengthSq( m_lastFormationAcceleration ) != 0 )
	{
		auto c = Normalize( Cross( m_lastFormationAcceleration, accelNormalized ) );
		if( Length( c ) == 0 )
		{
			c = Vector3( 0, 1, 0 );
		}
		auto angle = AngleFromNormalized( m_lastFormationAcceleration, accelNormalized );
		//float step = m_maxFormationRotationSpeed * deltaTime; TODO inject max-rotation speed into formula
		float step = ( 0.1f + 2.f / float( m_formationGrid.size() ) ) * deltaTime;
		angle = min( angle, step );

		if( angle > 0 )
		{
			auto quat = RotationQuaternion( c, angle );
			TriVectorRotateQuaternion( &acceleration, &m_lastFormationAcceleration, &quat );

			for( auto pos = m_formationGrid.begin(); pos != m_formationGrid.end(); ++pos )
			{
				//float len = Length( *pos );
				TriVectorRotateQuaternion( &*pos, &( *pos ), &quat );
			}
		}
	}
	m_lastFormationAcceleration = Normalize( acceleration );
}

void Formation::UpdateAgents( std::vector<DroneAgent>& agents, void* scratchData, float radius )
{
	auto data = static_cast<FormationData*>( scratchData );
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, data++ )
	{
		agent->acceleration *= 0.5; //reduce the effect of former behaviors

		Vector3 vectToSlot = m_formationPosition + m_formationGrid[data->assignedSlot] - agent->position;

		float distToSlot = LengthSq( vectToSlot );
		float radSq = radius * radius;

		if( distToSlot < 2 * radSq )
		{
			// drones can slow down based on dist to target
			agent->velocity *= 0.5f + 0.5f * min( max( distToSlot / radSq - 0.5f * radius, 0.f ), 1.0f );
		}

		agent->acceleration += Normalize( vectToSlot ) * m_behaviorWeight;
	}
}

void Formation::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Formation" );
}

void Formation::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "Formation" ) && m_isFormalizing )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_formationPosition ) * parentWorldLocation, 20, 8, Tr2DebugRenderer::Wireframe, 0xffff1111 );

		renderer.DrawSphere( this, TranslationMatrix( m_formationPosition + Normalize( m_lastFormationAcceleration ) * 100 ) * parentWorldLocation, 20, 8, Tr2DebugRenderer::Wireframe, 0xffff1111 );

		for( auto point = m_formationGrid.begin(); point != m_formationGrid.end(); ++point )
		{
			renderer.DrawSphere( this, TranslationMatrix( m_formationPosition + *point ) * parentWorldLocation, 20, 8, Tr2DebugRenderer::Wireframe, 0xffff1111 );
		}
	}
}
