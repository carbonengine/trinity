#include "StdAfx.h"
#include "include/TriMath.h"
#include "SeekTarget.h"

SeekTarget::SeekTarget( IRoot* lockobj ) :
	m_behaviorWeight( 20.f ),
	m_arrivedRadius( 10.f ),
	m_slowDownRadius( 33.f ),
	m_distanceFromShip( 50.f ),
	m_seconds( 0.25f ),
	m_exit( false ),
	m_droneArrived( false ),
	m_target( nullptr ),
	m_tunnelBehavior( nullptr ),
	m_fxBehavior( nullptr )
{
}

SeekTarget::~SeekTarget()
{
}

size_t SeekTarget::GetScratchMemorySize() const
{
	return sizeof( LocatorData );
}

void SeekTarget::InitializeScratch( void* scratchMemory )
{
	*static_cast<LocatorData*>( scratchMemory ) = LocatorData();
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

	auto data = static_cast<LocatorData*>( scratchData );
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		// If drone does not have a picked locator, then pick one
		if( agent->target == Vector3( 0, 0, 0 ) )
		{
			unsigned int count = m_target->GetDamageLocatorCount();
			int rand = TriRandInt( count );
			data->index = rand;
			m_target->GetDamageLocatorDirection( &data->direction, data->index, true );
		}

		// Want to keep this updated because the ship might be moving (upon dock)
		m_target->GetDamageLocatorPosition( &data->position, data->index, true );

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

void SeekTarget::SetTarget( ITriTargetable* target )
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