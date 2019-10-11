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
		auto lastAccelNormalized = Normalize( *data );
		auto lastAccelLength = Length( *data );
		auto accelNormalized = Normalize( agent->acceleration );
		auto accelLength = Length( agent->acceleration );
		agent->target = agent->acceleration;

		if ( LengthSq( lastAccelNormalized ) != 0 && m_maxRotationSpeed > 0 )
		{
			auto c = Normalize( Cross( lastAccelNormalized, accelNormalized ) );
			if ( Length( c ) == 0 )
			{
				c = Vector3( 0, 1, 0 );
			}
			auto angle = AngleFromNormalized( lastAccelNormalized, accelNormalized );
			float step = m_maxRotationSpeed * deltaTime;
			angle = min( angle, step );
			if ( angle > 0 )
			{
				auto quat = RotationQuaternion( c, angle );
				TriVectorRotateQuaternion( &agent->acceleration, &lastAccelNormalized, &quat);
			}

			
			// TODO this might need to be modified to act more naturally when forces flip directions i.e. bounce of walls
			// or activate thrusters etc since length of lastAccel and Accel might be equal but the change is actually accel x2
			 agent->acceleration = Normalize(agent->acceleration) * Lerp(lastAccelLength, accelLength,
			                                                            TriClamp(m_inertiaWeight, 0, 1) * deltaTime);

			// clamp to try to limit speedups more can be removed with the rework listed above
			 agent->acceleration = ClampLength( agent->acceleration, m_maxAcceleration );

		}
		*data = agent->acceleration;
	}
	std::vector<Vector3> noNeedToReturnForces;
	return noNeedToReturnForces;
}

void Inertia::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation)
{
}