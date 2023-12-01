#pragma once
#ifndef CollisionAvoidance_H
#define CollisionAvoidance_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "Eve/Volume/IEveVolume.h"

BLUE_DECLARE_INTERFACE( IEveVolume );

BLUE_CLASS( CollisionAvoidance ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	CollisionAvoidance( IRoot* lockobj = nullptr );
	~CollisionAvoidance();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	int GetProcessPriority();

private:
	bool m_enabled;
	PIEveVolumeVector m_exclusionVolumes;
	float m_collisionAvoidanceScalar;
	int32_t m_priority;
};

TYPEDEF_BLUECLASS( CollisionAvoidance );

#endif