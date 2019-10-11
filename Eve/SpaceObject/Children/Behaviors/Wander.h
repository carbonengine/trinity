#pragma once
#ifndef Wander_H
#define Wander_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( Wander ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Wander( IRoot* lockobj = nullptr );
	~Wander();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );

private:
	float m_circleDistance;
	float m_circleRadius;
	float m_weightWander;	//priority of behavior

};

TYPEDEF_BLUECLASS( Wander );

#endif