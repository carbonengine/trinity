#include "StdAfx.h"
#include "Inertia.h"
#include "include/TriMath.h"

Inertia::Inertia( IRoot* lockobj ) :
	m_inertiaWeight( 0.5 ),
	m_maxRotationSpeed( 3.14 ),
	m_maxAcceleration( 2 )
{
}

Inertia::~Inertia()
{
}

int Inertia::GetProcessPriority()
{
	return PROCESS_INERTIA;
}

size_t Inertia::GetScratchMemorySize() const
{
	return sizeof( Vector3 );
}

void Inertia::InitializeScratch( const DroneAgent& drone, void* scratchMemory )
{
	*static_cast<Vector3*>( scratchMemory ) = Vector3( 0, 0, 0 );
}

std::vector<Vector3> Inertia::CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
                                                BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	auto data = static_cast<Vector3*>( scratchData );

	for (auto agent = agents.begin(); agent != agents.end(); ++agent, data++)
	{
		auto lastVelocityNormalized = Normalize( *data );
		auto lastVelocityLength = Length( *data );
		
		auto desiredVelocity = *data + agent->acceleration;
		auto velocityNormalized = Normalize( desiredVelocity );
		auto velocityLength = Length( desiredVelocity );

		if ( LengthSq( velocityNormalized ) != 0 )
		{
			if ( m_maxRotationSpeed > 0 )
			{
				auto c = Normalize( Cross( lastVelocityNormalized, velocityNormalized ) );
				if ( Length( c ) != 0 )
				{
					auto angle = acos( TriClamp( Dot( velocityNormalized, lastVelocityNormalized ), -1, 1 ) );
					angle = min( abs( angle ), m_maxRotationSpeed ) * deltaTime * ( angle >= 0 ? 1 : -1 );
					auto quat = RotationQuaternion( c, angle );
					TriVectorRotateQuaternion( &velocityNormalized, &lastVelocityNormalized, &quat);
					velocityNormalized = Normalize( velocityNormalized );
				}
			}
			// This might need to be modified to act more naturally when forces flip directions i.e. bounce of walls
			// or activate thrusters etc since length of lastAccel and Accel might be equal but the change is actually accel x2
			desiredVelocity = ( velocityNormalized * Lerp( velocityLength, lastVelocityLength, TriClamp( m_inertiaWeight, 0, 1 ) ) );
			agent->acceleration = desiredVelocity - agent->velocity;
		}
		*data = desiredVelocity;
	}
	std::vector<Vector3> noNeedToReturnForces;
	return noNeedToReturnForces;
}

void Inertia::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation)
{
}