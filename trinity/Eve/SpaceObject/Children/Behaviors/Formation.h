// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Formation_H
#define Formation_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

struct FormationData
{
	FormationData() :
		assignedSlot( -1 )
	{
	}

	int assignedSlot;
};

BLUE_CLASS( Formation ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Formation( IRoot* lockobj = nullptr );
	~Formation();

	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );

	int GetProcessPriority();
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent> & agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );

	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, std::vector<DroneAgent> & agents, Matrix & parentWorldLocation );
	std::string GetBehaviorName();
	bool InFormation();
	void Reset();

private:
	bool m_enabled;

	void InitializeFormation( std::vector<DroneAgent> & agents, void* scratchData, float radius );
	void CreateFormationGrid( std::vector<DroneAgent> & agents, Vector3 & targetDir, float radius );
	void AssignSlots( std::vector<DroneAgent> & agents, void* scratchData );
	void BreakFormation();
	void UpdateFormation( const float deltaTime, BehaviorGroup& group );
	void calculateFormationInertia( Vector3 & acceleration, const float deltaTime );
	void UpdateAgents( std::vector<DroneAgent> & agents, void* scratchData, float radius );
	bool CheckIfFormalizing( std::vector<DroneAgent> & agents, void* scratchData );

	std::vector<Vector3> m_formationGrid;
	std::vector<bool> m_formationGridReserver;
	bool m_inFormation;
	bool m_isFormalizing;
	Vector3 m_formationPosition;
	float m_maxFormationVelocityScaler;

	int32_t m_framesBetweenUpdates;
	int32_t m_frameCounter;
	int32_t m_stubbornness;
	int32_t m_stubbornnessCounter;
	int32_t m_priority;
	float m_behaviorWeight;
	float m_visionRange;
	float m_maxFormationRotationSpeed;
	std::vector<Vector3> m_lastPullForces;
	std::vector<Vector3> m_debugPoints;
	Vector3 m_formationSpeed;
	Vector3 m_formationRotation;
	Vector3 m_formationAcceleration;
	Vector3 m_lastFormationAcceleration;
};
TYPEDEF_BLUECLASS( Formation );

#endif