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

	void GridToggleReset();
	void UpdateGrid( BehaviorGroup & group );


private:
	bool m_enabled;
	bool m_addByCount;
	bool m_addOnGrid;
	bool m_regenerateDrones;
	float m_seconds;
	float m_time;
	float m_gridFullnessFactor; // we sometimes don't want a perfect grid so randomly decide when not to spawn some drones
	int m_count;
	Vector3 m_spawnPosition;
	Vector4 m_gridInfo; // x = x count, y = y count, z = z count, w = distance between (if gridspacing is not set)
	Vector3 m_gridSpacing;
};

TYPEDEF_BLUECLASS( SpawnDrones );

#endif
