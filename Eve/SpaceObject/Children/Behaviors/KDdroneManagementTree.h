#pragma once
#ifndef KDdroneManagementTree_H
#define KDdroneManagementTree_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"

enum planeType
{
	X = 0,
	Y = 1,
	Z = 2,
};

struct DroneAgent;

struct AgentRef
{
	AgentRef() :
		pos( nullptr ),
		agentID( 0 ),
		planeType( X ),
		Left( nullptr ),
		Right( nullptr )
	{}

	Vector3* pos;
	int agentID;
	planeType planeType;
	AgentRef* Left;
	AgentRef* Right;
};

struct compareRef
{
	bool operator()( const AgentRef& lhs, const AgentRef& rhs ) const
	{
		switch ( rhs.planeType )
		{
		case X:
			return lhs.pos->x < rhs.pos->x;
			break;
		case Y:
			return lhs.pos->y < rhs.pos->y;
			break;
		case Z:
			return lhs.pos->z < rhs.pos->z;
			break;
		}
		return false;
	}
};

BLUE_CLASS( KDdroneManagementTree ): public IRoot
{
public:
	EXPOSE_TO_BLUE();
	KDdroneManagementTree( IRoot* lockobj = nullptr );
	~KDdroneManagementTree();

	void AddAgentToTree( DroneAgent& );
	void CreateTree(std::vector<DroneAgent>& agents);
	void UpdateTree();
	void RenderDebugInfo(Tr2DebugRenderer& renderer, float debugSquareSize, Matrix& parentWorldLocation);

private:
	
	AgentRef* SplitSort(std::vector<AgentRef>& agents, int b, int e, planeType pt);
	std::vector<AgentRef> ChangeAgentsIntoAgentRefs(std::vector<DroneAgent>& agents);
	AgentRef ChangeAgentIntoAgentRef(DroneAgent& agent);
	planeType FindNextSplitAxis(planeType pt);
	std::vector<AgentRef>& sortByAxis(std::vector<AgentRef>& agents, int b, int e, planeType pt);
	void DrawTree(Tr2DebugRenderer& renderer, AgentRef* tree, Vector3 debugSquareCorner1, Vector3 debugSquareCorner2, Matrix&
	              parentWorldLocation);
	void DrawSquareInnerLines(Tr2DebugRenderer& renderer, Vector3 agentPos, Vector3 P1, Vector3 P2, Color C, planeType pt, Matrix&
	                          parentWorldLocation);
	float m_freq;
	AgentRef m_tree;
	std::vector< AgentRef > m_agentRefs;
};

TYPEDEF_BLUECLASS( KDdroneManagementTree );

#endif