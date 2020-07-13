#pragma once
#ifndef SpawnDrones_H
#define SpawnDrones_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( SpawnDrones ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	SpawnDrones( IRoot* lockobj = nullptr );
	~SpawnDrones();

	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
		BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );

	float m_seconds;
	float m_time;
	Vector3 m_spawnPosition;
};

TYPEDEF_BLUECLASS( SpawnDrones );

#endif
