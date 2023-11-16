#pragma once
#ifndef InclusionVolume_H
#define InclusionVolume_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "Eve/Volume/IEveVolume.h"

BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );

BLUE_CLASS( InclusionVolume ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	InclusionVolume( IRoot* lockobj = nullptr );
	~InclusionVolume();

	virtual int GetProcessPriority();
	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );

private:
	bool m_enabled;
	PIEveVolumeVector m_inclusionVolumes;
	int32_t m_priority;
	int32_t m_framesBetweenUpdates;
	int32_t m_frameCounter;
	std::vector<Vector3> m_lastPullForces;
	float m_behaviorWeight;
};
TYPEDEF_BLUECLASS( InclusionVolume );

#endif