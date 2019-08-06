#pragma once
#ifndef FollowASpline_H
#define FollowASpline_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_DECLARE( TriCurveSet );

BLUE_CLASS( FollowASpline ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	FollowASpline( IRoot* lockobj = nullptr );
	~FollowASpline();
	
	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system);
	void RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);

private:
	float ProcessTunnelEntrances(DroneAgent& agent, std::vector<SplineTunnel>& tunnels);
	void ProcessAssignedTunnel(DroneAgent& agent, std::vector<SplineTunnel>& tunnels, BehaviorGroup& group);

	TunnelGroupType m_tunnelGroupType;
	float m_behaviorWeight;	
	float m_smoothPullFactor;
	float m_cornerSmoothener;
	Vector3 m_desiredVector;
	std::vector <Vector3> m_targetPointVector;
};

TYPEDEF_BLUECLASS( FollowASpline );

#endif
