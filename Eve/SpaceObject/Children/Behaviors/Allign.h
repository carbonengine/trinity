#pragma once
#ifndef Allign_H
#define Allign_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"


BLUE_CLASS( Allign ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Allign( IRoot* lockobj = nullptr );
	~Allign();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	float GetBehaviorSearchRadius();

private:
	int m_framesBetweenUpdates;
	int m_frameCounter;
	std::vector<Vector3> m_lastPullForces;
	float m_behaviorWeight;
	float m_visionRange;
};
TYPEDEF_BLUECLASS( Allign );

#endif