#pragma once
#ifndef DroneAvoidance_H
#define DroneAvoidance_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"


BLUE_CLASS( DroneAvoidance ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	DroneAvoidance( IRoot* lockobj = nullptr );
	~DroneAvoidance();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	float GetBehaviorSearchRadius();
	virtual int GetProcessPriority();

private:
	bool m_enabled;
	int32_t m_framesBetweenUpdates;
	int32_t m_frameCounter;
	std::vector<Vector3> m_lastPullForces;
	float m_behaviorWeight;
	float m_visionRange;
	int32_t m_priority;
};
TYPEDEF_BLUECLASS( DroneAvoidance );

#endif