#include "StdAfx.h"
#include "include/TriMath.h"
#include "SeekTarget.h"

const BlueSharedString LOCATOR_SET_NAME( "seek" );

SeekTarget::SeekTarget( IRoot* lockobj ) :
	m_behaviorWeight( 20.f ),
	m_arrivedRadius( 10.f ),
	m_slowDownRadius( 33.f ),
	m_seconds( 0.35f ),
	m_counter( 0 ),
	m_repairTimePassed( 0.f ),
	m_totalRepairTime( -1.f ),
	m_exit( false ),
	m_repair( false ),
	m_droneArrived( false ),
	m_sortedLocators( false ),
	m_doneRepairing( false ),
	m_target( nullptr ),
	m_fxBehavior( nullptr ),
	m_locatorSetName( "damage" ),
	m_startTimer( false ),
	m_priority( LEAST_PRIORITY )
{
	m_locatorSet.CreateInstance();
}

SeekTarget::~SeekTarget()
{
}

int SeekTarget::GetProcessPriority()
{
	return m_priority;
}


size_t SeekTarget::GetScratchMemorySize() const
{
	return sizeof( SeekTargetData );
}

void SeekTarget::InitializeScratch( void* scratchMemory )
{
	*static_cast<SeekTargetData*>( scratchMemory ) = SeekTargetData();
}

std::vector<Vector3> SeekTarget::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	if( m_behaviorWeight <= 0 )
	{
		return m_todo;
	}

	if( m_fxBehavior == nullptr )
	{
		m_fxBehavior = group.GetBehaviorByName( "PlayFX" );
	}

	auto data = static_cast<SeekTargetData*>( scratchData );

	// Make sure the drones stop seeking ship locators when done repairing
	if( m_exit && m_counter >= 1 )
	{
		m_doneRepairing = true;
		m_exit = false;
		m_counter = 0;
	}

	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		if( m_totalRepairTime != -1.f && m_repairTimePassed >= m_totalRepairTime )
		{
			SetExit( true );
			m_repair = false;
			m_repairTimePassed = 0.f;
		}

		if( m_doneRepairing )
		{
			data->arrived = true;
		}

		if( m_repair == true && m_target != nullptr )
		{
			// If drone does not have a picked locator, then pick one
			if( agent->target == Vector3( 0, 0, 0 ) || data->locatorIndex == -1 )
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
		}
		else
		{
			if( data->arrived && agent->target == Vector3( 0, 0, 0 ) )
			{
				//Get count of locators under the "seek" locatorSet
				auto seekLocators = GetLocatorsForSet( LOCATOR_SET_NAME );
				if( seekLocators != NULL && seekLocators[0].size() > 0 )
				{
					int rand = TriRandInt( 0, (int)seekLocators->size() );
					data->position = seekLocators[0][rand].position;
					data->direction = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), seekLocators[0][rand].direction );
				}
				data->arrived = false;
			}
		}

		agent->target = data->position;

		// Set the target point on the radius sphere
		Vector3 fakePoint = data->direction;
		fakePoint = Normalize( fakePoint );

		if( m_repair == true && m_target != nullptr )
		{
			Vector3 center;
			Vector3 radius;
			m_target->GetShapeEllipsoid( center, radius );
			fakePoint *= radius * 1.2;
		}
		else
		{
			fakePoint *= m_arrivedRadius;
		}

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
				m_droneArrived = true;
				m_doneRepairing = false;
				if( m_repair )
				{
					m_startTimer = true;
				}
			}

			// Set the rotation of the drone
			Quaternion newRotation;
			auto invDir = Normalize( data->position - agentPositionWS );
			TriQuaternionRotationArc( &newRotation, &zAxis, &invDir );
			agent->rotation = newRotation;
			data->timePassed = 0.f;

			// If the target has arrived then start playing effect
			if( distance < m_arrivedRadius )
			{
				data->arrived = true;

				if( !agent->playFX && m_fxBehavior != nullptr )
				{
					agent->fxStartTime = BeOS->GetActualTime();
					agent->playFX = true;
				}
			}
		}
		else
		{
			// Have the drone slowly start moving based on time passed
			data->timePassed += deltaTime;
			data->timePassed = max( data->timePassed, m_seconds );
			desiredVelocity *= Lerp( 0, 1, max( data->timePassed, m_seconds ) / m_seconds );
		}

		// Trigger if: Repairing Ship && Player Undocks
		if( m_exit )
		{
			agent->playFX = false;
			m_repair = false;
			data->locatorIndex = -1;
			m_droneArrived = false;
			m_counter++;
			m_startTimer = false;
		}

		agent->acceleration += desiredVelocity - agent->velocity;
	}

	m_doneRepairing = false;

	if( m_startTimer )
	{
		m_repairTimePassed += deltaTime;
	}

	return m_todo;
}

// --------------------------------------------------------------------------------
// Description:
//   Try to find the specified locator set and return a pointer to it
// --------------------------------------------------------------------------------
const LocatorStructureList* SeekTarget::GetLocatorsForSet( const BlueSharedString& setName ) const
{
	if( m_locatorSet->HasName( setName ) )
	{
		return m_locatorSet->GetLocators();
	}
	return nullptr;
}

void SeekTarget::SetTarget( EveSpaceObject2* target )
{
	m_target = target;
}

void SeekTarget::SetTotalRepairTime( float seconds )
{
	m_totalRepairTime = seconds;
}

void SeekTarget::SetExit( bool value )
{
	m_exit = value;
}

void SeekTarget::SetBehaviorWeight( float value )
{
	m_behaviorWeight = value;
}

void SeekTarget::ResetBehavior()
{
	m_counter = 0;
	m_exit = false;
	m_repair = false;
	m_droneArrived = false;
	m_doneRepairing = true;
}

void SeekTarget::SetupShipRepair()
{
	if( m_fxBehavior )
	{
		m_fxBehavior->UpdateState( false );
	}

	m_exit = false;
	m_droneArrived = false;
	m_repair = true;
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

	if( desiredLength == 0.f )
	{
		return;
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
	options.insert( "Locators" );
}

void SeekTarget::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "SeekTarget" ) )
	{
		renderer.DrawSphere( this, m_arrivalPoint, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffffffff );
		renderer.DrawSphere( this, m_arrivalPoint, 5, 8, Tr2DebugRenderer::Wireframe, 0xff0000ff );
		renderer.DrawSphere( this, m_arrivalPoint, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xffcc11ff );
	}

	if( renderer.HasOption( this, "Locators" ) )
	{
		float boundingSphereRadius = 50.f;
		float modelScale = 5;

		const LocatorStructureList& locators = *m_locatorSet->GetLocators();
		for( size_t i = 0; i < locators.size(); ++i )
		{
			auto& locator = locators[i];
			auto position = locator.position;
			auto rotation = locator.direction;
			uint32_t color = 0x990088ff;

			renderer.DrawSphereArrow(
				Tr2DebugObjectReference( &locators, uint32_t( i ) ),
				Vector3( XMVector3TransformCoord( position, parentWorldLocation ) ),
				Vector3( XMVector3TransformNormal( Vector3( 0, 1, 0 ), Matrix( XMMatrixRotationQuaternion( rotation ) ) * parentWorldLocation ) ),
				boundingSphereRadius * modelScale / 50.f,
				8,
				Tr2DebugRenderer::Lit,
				color );
		}
	}
}

void SeekTarget::AddLocatorSet()
{
	EveLocatorSetsPtr seekSet;
	seekSet.CreateInstance();
	seekSet->Set( "seek", NULL, 0 );

	m_locatorSet = seekSet;
}