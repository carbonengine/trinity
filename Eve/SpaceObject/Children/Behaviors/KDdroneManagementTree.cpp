#include "StdAfx.h"
#include "KDdroneManagementTree.h"
#include "Include/TriMath.h"
#include "include/TriQuaternion.h"

KDdroneManagementTree::KDdroneManagementTree(IRoot* lockobj) :
	m_freq(66.f)
{
}

KDdroneManagementTree::~KDdroneManagementTree()
{
}

void KDdroneManagementTree::AddAgentToTree( DroneAgent& agent)
{
	//agent.
}

void KDdroneManagementTree::CreateTree(std::vector<DroneAgent>& agents)
{
	if(agents.empty())
	{
		return;
	}

	ChangeAgentsIntoAgentRefs( agents );
	AgentRef tree = *SplitSort( m_agentRefs, 0, static_cast< int > ( agents.size() ) -1, X);
	m_tree = tree;
}

void KDdroneManagementTree::UpdateTree()
{
	
}

// agent = entire set, b = beginning of range, e = end of range, pt = what plane it should sort by
AgentRef* KDdroneManagementTree::SplitSort(std::vector<AgentRef>& agents, int b, int e, planeType pt)
{
	if( b == e )
	{
		return &agents[b];
	}

	if( b > e)
	{
		return nullptr;
	}

	auto arrayOfAgents = sortByAxis( agents, b, e + 1, pt );

	int m = b + static_cast< int >(floor( (e - b) / 2.f )); // middlePoint

	pt = FindNextSplitAxis( pt );
	agents[ m ].Left = SplitSort( agents, b,m - 1, pt );
	agents[ m ].Right = SplitSort( agents, m + 1, e, pt );

	AgentRef* point = &agents[m];
	return point;
}

std::vector<AgentRef> KDdroneManagementTree::ChangeAgentsIntoAgentRefs( std::vector<DroneAgent>& agents)
{
	
	for ( auto ag = agents.begin(); ag != agents.end(); ++ag )
	{
		AgentRef newRef;
		newRef.pos = &(ag->position);
		newRef.agentID =  ag->id;
		m_agentRefs.push_back( newRef );
	}
	return m_agentRefs;
}

AgentRef KDdroneManagementTree::ChangeAgentIntoAgentRef( DroneAgent& agent )
{
	AgentRef agentRef;

	agentRef.Right = nullptr;
	agentRef.Left = nullptr;
	agentRef.pos = &agent.position;
	agentRef.agentID = agent.id;
	
	return agentRef;
}

planeType KDdroneManagementTree::FindNextSplitAxis( planeType pt )
{
	switch ( pt )
	{
	case X:
		return Y;
		break;
	case Y:
		return Z;
		break;
	case Z:
		return X;
		break;
	}
	return X;
}

std::vector<AgentRef>& KDdroneManagementTree::sortByAxis(std::vector<AgentRef>& agents, int b , int e, planeType pt)
{
	for ( auto ag = agents.begin() + b; ag != agents.begin() + e; ++ag )
	{
		ag->planeType = pt;
	}

	std::sort( agents.begin() + b, agents.begin() + e, compareRef() );

	return agents;
}


void KDdroneManagementTree::RenderDebugInfo( Tr2DebugRenderer& renderer, float debugSquareSize, Matrix& parentWorldLocation )
{
	debugSquareSize = abs( debugSquareSize );
	Vector3 debugSquareCorner1 = Vector3( debugSquareSize , debugSquareSize , debugSquareSize );
	Vector3 debugSquareCorner2 = debugSquareCorner1 * -1;

	renderer.DrawBox( nullptr, debugSquareCorner1, debugSquareCorner2, Tr2DebugRenderer::Wireframe, 0xff555555 );
	
	DrawTree( renderer, &m_tree, debugSquareCorner1, debugSquareCorner2, parentWorldLocation );
	
}

void KDdroneManagementTree::DrawTree( Tr2DebugRenderer& renderer, AgentRef* tree,  Vector3 debugSquareCorner1, Vector3 debugSquareCorner2, Matrix& parentWorldLocation )
{
	if( tree == nullptr )
	{
		return;
	}

	if( tree->Left == nullptr && tree->Right == nullptr )
	{
		return;
	}

	Vector3 newCorner1;
	Vector3 newCorner2;
	switch ( (tree)->planeType )
	{
	case X:
		newCorner1 = Vector3( ( tree )->pos->x + .1f, debugSquareCorner1.y , debugSquareCorner1.z );
		newCorner2 = Vector3( ( tree )->pos->x - .1f, debugSquareCorner2.y, debugSquareCorner2.z );
		renderer.DrawBox( nullptr, newCorner1, newCorner2, Tr2DebugRenderer::Wireframe, 0xffff2222 );
		DrawSquareInnerLines( renderer, *tree->pos, newCorner1, newCorner2, 0xffff2222,X, parentWorldLocation);
		DrawTree( renderer, tree->Left, debugSquareCorner2, newCorner1, parentWorldLocation );
		DrawTree( renderer, tree->Right, newCorner2, debugSquareCorner1, parentWorldLocation );
		break;
	case Y:
		newCorner1 = Vector3( debugSquareCorner1.x, ( tree )->pos->y + .1f, debugSquareCorner1.z );
		newCorner2 = Vector3( debugSquareCorner2.x, ( tree )->pos->y - .1f, debugSquareCorner2.z );
		renderer.DrawBox( nullptr, newCorner1, newCorner2, Tr2DebugRenderer::Wireframe, 0xff22ff22 );
		DrawSquareInnerLines( renderer, *tree->pos, newCorner1, newCorner2, 0xff22ff22,Y, parentWorldLocation);
		DrawTree( renderer, ( *tree ).Left, newCorner2, debugSquareCorner1, parentWorldLocation );
		DrawTree( renderer, ( *tree ).Right, debugSquareCorner2, newCorner1, parentWorldLocation );
		break;
	case Z:
		newCorner1 = Vector3( debugSquareCorner1.x, debugSquareCorner1.y, ( tree )->pos->z + .1f );
		newCorner2 = Vector3( debugSquareCorner2.x, debugSquareCorner2.y, ( tree )->pos->z - .1f );
		renderer.DrawBox( nullptr, newCorner1, newCorner2, Tr2DebugRenderer::Wireframe, 0xff2222ff );
		DrawSquareInnerLines( renderer, *tree->pos, newCorner1, newCorner2, 0xff2222ff,Z, parentWorldLocation);
		DrawTree( renderer, ( *tree ).Left, debugSquareCorner2, newCorner1, parentWorldLocation );
		DrawTree( renderer, ( *tree ).Right, newCorner2, debugSquareCorner1, parentWorldLocation );
		break;
	}
}

void KDdroneManagementTree::DrawSquareInnerLines( Tr2DebugRenderer& renderer, Vector3 agentPos, Vector3 P1, Vector3 P2, Color C, planeType pt, Matrix& parentWorldLocation )
{
	switch ( pt )
	{
	case X:
		renderer.DrawLine( nullptr, agentPos, P1, C );
		renderer.DrawLine( nullptr, agentPos, P2, C );
		renderer.DrawLine( nullptr, agentPos, Vector3(agentPos.x, P1.y, P2.z), C );
		renderer.DrawLine( nullptr, agentPos, Vector3( agentPos.x, P2.y, P1.z ), C );
		break;
	case Y:
		renderer.DrawLine( nullptr, agentPos, P1, C );
		renderer.DrawLine( nullptr, agentPos, P2, C );
		renderer.DrawLine( nullptr, agentPos, Vector3( P1.x, agentPos.y, P2.z ), C );
		renderer.DrawLine( nullptr, agentPos, Vector3( P2.x, agentPos.y, P1.z ), C );
		break;
	case Z:
		renderer.DrawLine( nullptr, agentPos, P1, C );
		renderer.DrawLine( nullptr, agentPos, P2, C );
		renderer.DrawLine( nullptr, agentPos, Vector3( P1.x, P2.y, agentPos.z ), C );
		renderer.DrawLine( nullptr, agentPos, Vector3( P2.x, P1.y, agentPos.z ), C );
		break;
	}
}