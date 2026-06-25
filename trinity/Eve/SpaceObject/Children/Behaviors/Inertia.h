// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Inertia_H
#define Inertia_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

struct InertiaData
{
	InertiaData() :
		agentAccel( 0.0, 0.0, 0.0 ),
		inertiaWeight( 0.f )
	{
	}

	Vector3 agentAccel;
	float inertiaWeight;
};


BLUE_CLASS( Inertia ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Inertia( IRoot* lockobj = nullptr );
	~Inertia();

	virtual int GetProcessPriority();
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent> & agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, std::vector<DroneAgent> & agents, Matrix & parentWorldLocation );

private:
	bool m_enabled;
	float m_maxAcceleration;
	float m_minInertiaWeight; // We always want some inertia when the agent is at full speed
	float m_maxRotationSpeed;
	int32_t m_priority;
};
TYPEDEF_BLUECLASS( Inertia );

#endif
