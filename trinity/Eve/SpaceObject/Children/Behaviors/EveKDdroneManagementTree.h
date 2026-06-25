// Copyright © 2019 CCP ehf.

#pragma once
#ifndef EveKDdroneManagementTree_H
#define EveKDdroneManagementTree_H

#include "Tr2DebugRenderer.h"
#include "DroneAgent.h"
#include <iostream>

BLUE_CLASS( EveKDdroneManagementTree ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	EveKDdroneManagementTree( IRoot* lockobj = nullptr );
	~EveKDdroneManagementTree();

	void UpdateTree( const float dt );
	DroneAgent* FindClosestAgent( const Vector3 pos );
	void CreateTree( std::vector<DroneAgent> & agents, size_t BNbr );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, Matrix & parentWorldLocation );
	const std::vector<std::vector<std::vector<DroneAgent*>>>* FindDronesInRange(
		std::vector<DroneAgent> & agents, std::vector<float> & ranges, const float& BehaviorGroupBoundingSphereRadius );

private:
	enum PlaneType
	{
		X = 0,
		Y = 1,
		Z = 2,
	};

	struct SearchRange
	{
		SearchRange() :
			behaviorNbr( 0 ),
			radius( 0 )
		{
		}
		int behaviorNbr;
		float radius;
	};

	struct closestDrone
	{
		closestDrone() :
			agent( nullptr ),
			rangeBetween( 0 )
		{
		}
		DroneAgent* agent;
		float rangeBetween;
	};

	struct AgentRef
	{
		AgentRef() :
			agent( nullptr ),
			planeType( X ),
			left( nullptr ),
			right( nullptr )
		{
		}

		DroneAgent* agent;
		PlaneType planeType;
		int b; // pointer to first index in the managed vector
		int e; // pointer to last index in the managed vector
		AgentRef* left;
		AgentRef* right;
	};

	struct compareRef
	{
		bool operator()( const AgentRef& lhs, const AgentRef& rhs ) const
		{
			return lhs.agent->position[rhs.planeType] < rhs.agent->position[rhs.planeType];
		}

		bool operator()( const SearchRange& lhs, const SearchRange& rhs ) const
		{
			return lhs.radius > rhs.radius;
		}
	};

	PlaneType FindNextSplitAxis( PlaneType pt );
	AgentRef* CompareNodeToChildren( AgentRef * node );
	AgentRef* SplitSort( int b, int e, PlaneType pt );
	bool IsBiggestOnAxis( AgentRef * node, float n, PlaneType pt );
	bool IsSmallestOnAxis( AgentRef * node, float n, PlaneType pt );
	void ChangeAgentsIntoAgentRefs( std::vector<DroneAgent> & agents );
	void FindClosestAgentRecursive( const Vector3& pos, AgentRef* currentNode, closestDrone& agent ) const;
	std::vector<AgentRef>& SortByAxis( std::vector<AgentRef> & agents, int b, int e, PlaneType pt ) const;
	void SearchThroughTree( std::vector<std::vector<std::vector<DroneAgent*>>> & closeAgents, AgentRef * node, std::vector<DroneAgent> & agents, const std::vector<SearchRange>& ranges, int& activeRange ) const;
	static void AddAgentToSearchLists( std::vector<std::vector<std::vector<DroneAgent*>>> & closeAgents, AgentRef * node, float dist, const std::vector<SearchRange>& ranges, int activeRange, int agentNbr );
	void SearchThroughTreeHelperFunction( std::vector<std::vector<std::vector<DroneAgent*>>> & closeAgents, AgentRef * node, DroneAgent & agent, const std::vector<SearchRange>& ranges, int& activeRange, int& c ) const;

	// debug
	void DrawDebugTree( ITr2DebugRenderer2 & renderer, AgentRef * tree, Vector3 & debugSquareCorner1, Vector3 & debugSquareCorner2, Vector3 & pwt );
	void DrawSquareInnerLines( ITr2DebugRenderer2 & renderer, Vector3 & agentPos, Vector3 & P1, Vector3 & P2, Color C, PlaneType pt, Vector3 & pwt );

	AgentRef m_tree;
	float m_debugSquareSize;
	float m_updateTimeCounter;
	size_t m_maxFoundPerAgent;
	float m_timeBetweenUpdate;
	std::vector<AgentRef> m_agentRefs;
	std::vector<std::vector<std::vector<DroneAgent*>>> m_groupSearchReturnInfoBlock;
};

TYPEDEF_BLUECLASS( EveKDdroneManagementTree );

#endif


// expand file to review