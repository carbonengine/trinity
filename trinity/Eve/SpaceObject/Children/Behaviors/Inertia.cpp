#include "StdAfx.h"
#include "Inertia.h"
#include "include/TriMath.h"

Inertia::Inertia( IRoot* lockobj ) :
	m_enabled( true ),
	m_minInertiaWeight( 0.1 ),
	m_maxRotationSpeed( 3.14 ),
	m_maxAcceleration( 60 ),
	m_priority( LEAST_PRIORITY )
{
}

Inertia::~Inertia()
{
}

int Inertia::GetProcessPriority()
{
	return m_priority;
}

size_t Inertia::GetScratchMemorySize() const
{
	return sizeof( InertiaData );
}

void Inertia::InitializeScratch( void* scratchMemory )
{
	*static_cast<InertiaData*>( scratchMemory ) = InertiaData();
}

std::vector<Vector3> Inertia::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto data = static_cast<InertiaData*>( scratchData );

	std::vector<Vector3> force;
	if( !m_enabled )
	{
		return force;
	}
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		auto lastAccelNormalized = Normalize( data->agentAccel );
		auto lastAccelLength = Length( data->agentAccel );
		auto accelNormalized = Normalize( agent->acceleration );
		auto accelLength = Length( agent->acceleration );

		if( LengthSq( lastAccelNormalized ) != 0 && m_maxRotationSpeed > 0 )
		{
			auto c = Normalize( Cross( lastAccelNormalized, accelNormalized ) );
			if( Length( c ) == 0 )
			{
				c = Vector3( 0, 1, 0 );
			}
			auto angle = AngleFromNormalized( lastAccelNormalized, accelNormalized );
			float step = m_maxRotationSpeed * deltaTime;
			angle = min( angle, step );
			if( angle > 0 )
			{
				auto quat = RotationQuaternion( c, angle );
				TriVectorRotateQuaternion( &agent->acceleration, &lastAccelNormalized, &quat );
			}

			// needs to be data
			float agentVelocityLength = Length(agent->velocity);

			data->inertiaWeight = group.GetMaxVelocity() - agentVelocityLength;

			data->inertiaWeight = TriClamp( data->inertiaWeight, 0.1, group.GetMaxVelocity() );

			agent->acceleration = ClampLength( Normalize( agent->acceleration ) * Lerp( lastAccelLength, accelLength, data->inertiaWeight * deltaTime ), m_maxAcceleration );
			force.push_back( agent->acceleration );
		}
		data->agentAccel = agent->acceleration;
	}
	return force;
}

void Inertia::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
}