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

	std::string GetBehaviorName();
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& sys, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);
	virtual int GetProcessPriority();

	void UpdateState( bool state ) { m_exit = state; }

	std::vector<Vector3> GetPotentialPoints();

	std::vector<Vector3> GetEntrancePoints();
	
private:
	void FindASpawnPoint( DroneAgent& agent, ProcessLifetimeData* data );
	bool ProcessTunnel(DroneAgent& agent, SplineTunnel& tunnel, int& pointID, float boundingSphere);
	void findAndAssignAnExitTunnel(DroneAgent& agent, ProcessLifetimeData* data);
	void UpdateTunnelRegistry();
	void ReassignTunnelIDsAndAddSystemTunnels( EveChildBehaviorSystem& system );

	float m_firstAgentLifetime; // debug visualization 
	PSplineTunnelGroupVector m_splineTunnels;
	std::vector<SplineTunnel*> m_privateTunnels;

	float m_behaviorWeight;
	float m_returningAge; // how old is the drone when it starts returning (-1 means it'll last forever)
	bool m_respawnAgentsOnDeath;
	bool m_shouldReassignTunnelIDs;
	bool m_exit;
	Vector3 m_desiredVector;
	int m_priority;
};

TYPEDEF_BLUECLASS( ProcessLifetime );

#endif
