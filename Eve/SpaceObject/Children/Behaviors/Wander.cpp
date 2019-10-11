#include "StdAfx.h"
#include "Wander.h"
#include "Include/TriMath.h"
#include "include/TriQuaternion.h"

Wander::Wander(IRoot* lockobj) :
	m_weightWander( 4.f ),
	m_circleDistance( 1.5f ),
	m_circleRadius( 25.f )
{
}

Wander::~Wander()
{
}

std::vector<Vector3> Wander::CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	// Looks good with inertia->maxRotationSpeed(0.7) & maxAcceleration(0.2) 

	std::vector<Vector3> forceVectors;
	for( auto agent = agents.begin(); agent != agents.end(); ++agent )
	{
		Vector3 desiredVector( 0, 0, 0 );
		Vector3 fwd;
		Vector3 z( 0, 0, 1 );
		TriVectorRotateQuaternion( &fwd, &z, &agent->rotation );
		// Calculate how far the circle should be from the agent's position
		Vector3 centerOfCircle = agent->position + fwd * m_circleDistance;

		Vector3 rightVector = (Normalize( Cross( fwd, z ) ) ) * m_circleRadius;

		// Double check this
		if( rightVector == Vector3( 0, 0, 0 ) )
		{
			rightVector = z;
		}

		float randomAngle = TriFloatRandom01() * XM_2PI;  //Random angle between 0 and 2PI radians
		
		Quaternion rightVectorRotation = RotationQuaternion( fwd, randomAngle );
		// Rotate the right vector by the quaternion
		TriVectorRotateQuaternion( &rightVector, &rightVector, &rightVectorRotation );

		rightVector *= m_circleRadius;

		desiredVector = rightVector - centerOfCircle;
		desiredVector = Normalize( desiredVector );
		

		if ( group.m_collectForces )
		{
			Vector3 forceOffset = desiredVector * group.GetBoundingSphereRadius();

			forceVectors.push_back( agent->position + forceOffset );
			forceVectors.push_back( desiredVector * m_weightWander );
		}

		desiredVector *= m_weightWander;
		// Apply the force to the acceleration
		agent->acceleration += desiredVector;
	}
	return forceVectors;
}

void Wander::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "Wander" ) )
	{
		for( auto agent = agents.begin(); agent != agents.end(); ++agent )
		{
			Vector3 fwd;
			Vector3 z( 0, 0, 1 );
			TriVectorRotateQuaternion( &fwd, &z, &agent->rotation );
			auto centerOfCircle = agent->position + fwd * m_circleDistance;

			Vector3 rightVector = ( Normalize( Cross( fwd, z) ) ) * m_circleRadius;

			renderer.DrawSphere( this, TranslationMatrix( centerOfCircle ) * parentWorldLocation, m_circleRadius, 6, Tr2DebugRenderer::Wireframe, 0xffff00ff );
			// From agent->pos to the sphere
			renderer.DrawLine( this, agent->position, centerOfCircle, 0xffff00ff );
		
			float randomAngle = TriFloatRandom01() * XM_2PI;  //Random angle between 0 and 2PI radians
			Quaternion rightVectorRotation = RotationQuaternion( fwd, randomAngle );
			TriVectorRotateQuaternion( &rightVector, &rightVector, &rightVectorRotation );

			rightVector = Normalize( rightVector );
			rightVector *= m_circleRadius;
			renderer.DrawLine( this, centerOfCircle, rightVector, 0xffff00ff );
		}
	}
}
