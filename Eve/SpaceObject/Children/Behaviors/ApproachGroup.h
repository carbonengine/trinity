#pragma once
#ifndef ApproachGroup_H
#define ApproachGroup_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"


BLUE_CLASS( ApproachGroup ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	ApproachGroup( IRoot* lockobj = nullptr );
	~ApproachGroup();

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
TYPEDEF_BLUECLASS( ApproachGroup );

#endif