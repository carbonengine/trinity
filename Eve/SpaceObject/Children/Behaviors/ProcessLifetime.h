#pragma once
#ifndef ProcessLifetime_H
#define ProcessLifetime_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"


struct ProcessLifetimeData
{
	ProcessLifetimeData() :
		hasUsedEntryTunnel( false ),
		hasUsedExitTunnel( false ),
		assignedLifeTimeTunnel( - 1 ),
		tunnelPoint( 0 )
	{
	}

	int unsigned assignedLifeTimeTunnel;
	int tunnelPoint;
	bool hasUsedEntryTunnel;
	bool hasUsedExitTunnel;
};

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( SplineTunnelGroup );
BLUE_DECLARE_VECTOR( SplineTunnelGroup );

BLUE_CLASS( ProcessLifetime ) :
	public IBehavior,
	public IListNotify,
	public INotify
{
public:
	EXPOSE_TO_BLUE();
	ProcessLifetime( IRoot* lockobj = nullptr );
	~ProcessLifetime();

	// Notify
	bool OnModified(Be::Var* value);
	void OnListModified(long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList);

	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( const DroneAgent& drone, void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& sys, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);
	
private:
	void FindASpawnPoint( DroneAgent& agent, ProcessLifetimeData* data );
	bool ProcessTunnel(DroneAgent& agent, SplineTunnel& tunnel, int& pointID, float boundingSphere);
	void findAndAssignAnExitTunnel(DroneAgent& agent, ProcessLifetimeData* data);
	void UpdateTunnelRegistry();
	void ReassignTunnelIDsAndAddSystemTunnels( EveChildBehaviorSystem& system );

	float m_firstAgentLifetime; // debug visualization 
	PSplineTunnelGroupVector m_splineTunnels;
	std::vector<SplineTunnel*> m_privateTunnels;

	bool m_respawnAgentsOnDeath;
	float m_behaviorWeight;
	bool m_shouldReassignTunnelIDs;
	float m_returningAge; // how old is the drone when it starts returning (-1 means it'll last forever)
	Vector3 m_desiredVector;
};

TYPEDEF_BLUECLASS( ProcessLifetime );

#endif
