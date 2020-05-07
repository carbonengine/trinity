#include "StdAfx.h"
#include "include/TriMath.h"
#include "SeekTarget.h"

SeekTarget::SeekTarget( IRoot* lockobj ) :
	m_behaviorWeight( 20.f ),
	m_arrivedRadius( 10.f ),
	m_slowDownRadius( 33.f ),
	m_distanceFromShip( 0.0f ),
	m_seconds( 0.25f ),
	m_exit( false ),
	m_droneArrived( false ),
	m_sortedLocators( false ),  
	m_target( nullptr ),
	m_tunnelBehavior( nullptr ),
	m_fxBehavior( nullptr ),
	m_locatorSetName( "damage" )
{
}

SeekTarget::~SeekTarget()
{
}

size_t SeekTarget::GetScratchMemorySize() const
{
	return sizeof( SeekTargetData );
}

void SeekTarget::InitializeScratch( void* scratchMemory )
{
	*static_cast<SeekTargetData*>( scratchMemory ) = SeekTargetData();
}

std::vector<Vector3> SeekTarget::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	// Make sure python gets called first
	if( m_target == nullptr || m_behaviorWeight <= 0 )
	{
		return m_todo;
	}

	if( m_tunnelBehavior == nullptr && m_fxBehavior == nullptr )
	{
		m_tunnelBehavior = group.GetBehaviorByName( "ProcessLifetime" );
		m_fxBehavior = group.GetBehaviorByName( "PlayFX" );
	}

	if( m_distanceFromShip == 0.0f )
	{
		SetDistanceFromShip( m_target->GetRadius() );
	}

	auto data = static_cast<SeekTargetData*>( scratchData );
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		// If drone does not have a picked locator, then pick one
		if( agent->target == Vector3( 0, 0, 0 ) )
		{
			if( m_sortedLocators )
			{
				data->bucketId = agent->id % m_boundingBoxes.size();
				auto bucket = m_locatorBucketIndices[data->bucketId];
				data->locatorIndex = TriRandInt( int( bucket.size() ) );
			}
			else
			{
				unsigned int count = m_target->GetLocatorCount( m_locatorSetName );
				int rand = TriRandInt( count );
				data->locatorIndex = rand;
			}
		}

		// Want to keep this updated because the ship might be moving (when docking)
		if( m_sortedLocators )
		{
			int locatorIndex = m_locatorBucketIndices[data->bucketId][data->locatorIndex];
			m_target->GetLocatorPosition( &data->position, locatorIndex, true, m_locatorSetName );
			m_target->GetLocatorDirection( &data->direction, locatorIndex, true, m_locatorSetName );
		}
		else
		{
			m_target->GetLocatorPosition( &data->position, data->locatorIndex, true, m_locatorSetName );
			m_target->GetLocatorDirection( &data->direction, data->locatorIndex, true, m_locatorSetName );
		}

		agent->target = data->position;
		
		// Set the target point on the radius sphere
		Vector3 fakePoint = data->direction;
		fakePoint = Normalize( fakePoint );
		fakePoint *= m_distanceFromShip;
		fakePoint += data->position;

		// For debugging
		m_arrivalPoint = fakePoint;

		Matrix worldTransform = system.GetWorldTransform();
		Vector3 agentPositionWS = XMVector3TransformCoord( agent->position, worldTransform );

		Vector3 desiredVelocity = fakePoint - agentPositionWS;
		float distance = Length( desiredVelocity );
		desiredVelocity = Normalize( desiredVelocity );
		static const Vector3 zAxis( 0.f, 0.f, 1.f );

		// If the agent is approaching, slow him down
		if( distance < m_slowDownRadius )
		{
			desiredVelocity = desiredVelocity * m_behaviorWeight * ( distance / m_slowDownRadius );

			if( !m_droneArrived && m_onFirstDroneArrivedCallback )
			{
				m_onFirstDroneArrivedCallback.CallVoid().ReportException();				
			}
			m_droneArrived = true;
			
			// Set the rotation of the drone
			Quaternion newRotation;
			auto invDir = Normalize(data->position - agentPositionWS );
			TriQuaternionRotationArc( &newRotation, &zAxis, &invDir );
			agent->rotation = newRotation;
			data->timePassed = 0.f;

			// If the target has arrived then start playing effect
			if( distance < m_arrivedRadius )
			{
				if( !agent->playFX )
				{
					agent->fxStartTime = BeOS->GetActualTime();
					agent->playFX = true;
				}
			}
			// Trigger if: Repairing Ship && Player Undocks
			else if( m_exit )
			{
				if( m_tunnelBehavior )
				{
					// Drones search for an exit tunnel
					m_tunnelBehavior->UpdateState( true );
				}

				if( m_fxBehavior )
				{
					// Behavior deletes effects
					m_fxBehavior->UpdateState( true );
				}

				agent->playFX = false;
				m_behaviorWeight = 0;
			}
		}
		else
		{
			// Have the drone slowly start moving based on time passed
			data->timePassed += deltaTime;
			data->timePassed = max( data->timePassed, m_seconds );
			desiredVelocity *= Lerp( 0, 1, max(data->timePassed, m_seconds ) / m_seconds );
		}
		agent->acceleration += desiredVelocity - agent->velocity;
	}

	return m_todo;
}

void SeekTarget::SetDistanceFromShip( float boundingRadius )
{
	m_distanceFromShip = boundingRadius / 3;
}

void SeekTarget::SetTarget( EveSpaceObject2* target )
{
	m_target = target;
}

void SeekTarget::SetExit( bool value )
{
	m_exit = value;
}

void SeekTarget::SetBehaviorWeight( float value )
{
	m_behaviorWeight = value;
}

void SeekTarget::ResetBehavior( )
{
	if( m_fxBehavior )
	{
		m_fxBehavior->UpdateState( false );
	}

	if( m_tunnelBehavior )
	{
		m_tunnelBehavior->UpdateState( false );
	}

	m_exit = false;
	m_droneArrived = false;
}

void SeekTarget::SplitBoundingBox()
{
	Vector3 mn, mx;
	if( !m_target->GetLocalBoundingBox( mn, mx ) )
	{
		return;
	}

	// Get width, height and length of bounding box
	Vector3 bb = mx - mn;

	float largest = -std::numeric_limits<float>::max();
	float secondLargest = -std::numeric_limits<float>::max();
	int maxIndex = -1;

	for( int i = 0; i < 3; ++i )
	{
		if( bb[i] > largest )
		{
			secondLargest = largest;
			largest = bb[i];
			maxIndex = i;
		}
		else if( bb[i] > secondLargest && bb[i] != largest )
		{
			secondLargest = bb[i];
		}
	}

	if( maxIndex == -1 )
	{
		return;
	}
	
	float desiredLength = largest;
	int boxCount = 0;

	Vector3 aabbMin = mn;
	Vector3 aabbMax = mx;

	while( desiredLength > secondLargest )
	{
		desiredLength *= 0.5;
	}

	boxCount = int( largest ) / int( desiredLength );
	for( int i = 0; i < boxCount; ++i )
	{
		aabbMin[maxIndex] = ( i * desiredLength ) + mn[maxIndex];
		aabbMax[maxIndex] = ( i + 1 ) * desiredLength + mn[maxIndex];
		AxisAlignedBoundingBox aabb = AxisAlignedBoundingBox( aabbMin, aabbMax );
		m_boundingBoxes.push_back( aabb );
	}

	m_locatorBucketIndices.resize( m_boundingBoxes.size() );

	SortLocators();
}

void SeekTarget::SortLocators()
{
	auto locators = m_target->GetLocatorsForSet( m_locatorSetName );
	if( !locators )
	{
		return;
	}

	for( int i = 0; i < m_locatorBucketIndices.size(); ++i )
	{
		for( int j = 0; j < int( locators->size() ); ++j )
		{
			if( m_boundingBoxes[i].IsPointInside( ( *locators )[j].position ) )
			{
				m_locatorBucketIndices[i].push_back( j );
			}
		}
	}

	// In case we have an empty bucket
	for( int i = 0; i < m_locatorBucketIndices.size(); )
	{
		if( m_locatorBucketIndices[i].size() == 0 )
		{
			m_locatorBucketIndices.erase( m_locatorBucketIndices.begin() + i );
		}
		else
		{
			++i;
		}
	}

	m_sortedLocators = true;
}

void SeekTarget::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "SeekTarget" );
}

void SeekTarget::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "SeekTarget" ) )
	{
		renderer.DrawSphere( this, m_arrivalPoint, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffffffff );
		renderer.DrawSphere( this, m_arrivalPoint, 5, 8, Tr2DebugRenderer::Wireframe, 0xff0000ff );
		renderer.DrawSphere( this, m_arrivalPoint, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xffcc11ff );
	}
}