#pragma once
#ifndef CollisionAvoidance_H
#define CollisionAvoidance_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( CollisionAvoidance ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	CollisionAvoidance( IRoot* lockobj = nullptr );
	~CollisionAvoidance();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );

private:
	float m_collisionAvoidanceScalar;
	float m_collisionStrength;
};

TYPEDEF_BLUECLASS( CollisionAvoidance );

#endif