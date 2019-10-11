#pragma once
#ifndef Inertia_H
#define Inertia_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( Inertia ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Inertia( IRoot* lockobj = nullptr );
	~Inertia();

	virtual int GetProcessPriority();
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( const DroneAgent& drone, void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
private:
	float m_maxAcceleration;
	float m_inertiaWeight;
	float m_maxRotationSpeed;
};
TYPEDEF_BLUECLASS( Inertia );

#endif