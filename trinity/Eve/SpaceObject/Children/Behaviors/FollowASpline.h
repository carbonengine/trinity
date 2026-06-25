// Copyright © 2023 CCP ehf.

#pragma once
#ifndef FollowASpline_H
#define FollowASpline_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

struct FollowASplineData
{
	FollowASplineData() :
		tunnelLock( -1 ),
		tunnelPoint( 0 )
	{
	}

	int tunnelLock;
	int tunnelPoint;
};

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( SplineTunnelGroup );
BLUE_DECLARE_VECTOR( SplineTunnelGroup );

BLUE_CLASS( FollowASpline ) :
	public IBehavior,
	public IListNotify,
	public INotify
{
public:
	EXPOSE_TO_BLUE();
	FollowASpline( IRoot* lockobj = nullptr );
	~FollowASpline();


	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent> & agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	bool OnModified( Be::Var * value );
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, std::vector<DroneAgent> & agents, Matrix & parentWorldLocation );
	virtual int GetProcessPriority();

private:
	bool m_enabled;

	float ProcessTunnelEntrances( DroneAgent & agent, const std::vector<SplineTunnel*>& tunnels, FollowASplineData* data );
	bool ProcessAssignedTunnel( DroneAgent & agent, const std::vector<SplineTunnel*>& tunnels, BehaviorGroup& group, FollowASplineData* data );
	bool CheckForAndUpdateFormation( std::vector<DroneAgent> & agents, BehaviorGroup & group, void* scratchData, int tunnel, int tunnelPoint );
	void ReassignTunnelIDsAndAddSystemTunnels( EveChildBehaviorSystem & system );
	void UpdateTunnelRegistry();

	PSplineTunnelGroupVector m_splineTunnels;
	std::vector<SplineTunnel*> m_privateTunnels;
	bool m_shouldReassignTunnelIDs;
	TunnelGroupType m_tunnelGroupType;
	float m_behaviorWeight;
	float m_smoothPullFactor;
	float m_cornerSmoothener;
	Vector3 m_desiredVector;
	std::vector<Vector3> m_targetPointVector;
	int m_framesBetweenUpdates;
	int m_frameCounter;
	std::vector<Vector3> m_lastPullForces;
	int32_t m_priority;
};

TYPEDEF_BLUECLASS( FollowASpline );

#endif
