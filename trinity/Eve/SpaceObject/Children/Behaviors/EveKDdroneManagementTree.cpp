// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveKDdroneManagementTree.h"

EveKDdroneManagementTree::EveKDdroneManagementTree( IRoot* lockobj ) :
	m_debugSquareSize( 0 ),
	m_updateTimeCounter( 0 ),
	m_timeBetweenUpdate( 1 ), //update every '1' seconds
	m_maxFoundPerAgent( 5 )
{
}

EveKDdroneManagementTree::~EveKDdroneManagementTree()
{
}

void EveKDdroneManagementTree::CreateTree( std::vector<DroneAgent>& agents, size_t NumberOfBehaviors )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( agents.empty() )
	{
		return;
	}

	ChangeAgentsIntoAgentRefs( agents );
	AgentRef tree = *SplitSort( 0, static_cast<int>( agents.size() ) - 1, Z );
	m_tree = tree;

	m_groupSearchReturnInfoBlock.clear();

	for( int j = 0; j < static_cast<int>( NumberOfBehaviors ); j++ )
	{
		std::vector<std::vector<DroneAgent*>> perAgentData;
		for( unsigned int i = 0; i < agents.size(); i++ )
		{
			perAgentData.push_back( std::vector<DroneAgent*>() );
		}
		m_groupSearchReturnInfoBlock.push_back( perAgentData );
	}
}

void EveKDdroneManagementTree::UpdateTree( const float dt )
{
	if( m_timeBetweenUpdate != -1 && m_updateTimeCounter >= m_timeBetweenUpdate )
	{
		m_updateTimeCounter = 0;
		m_tree = *CompareNodeToChildren( &m_tree );
	}
	else
	{
		m_updateTimeCounter += dt;
	}
}

// checking axis for if there needs to be a re-construction
EveKDdroneManagementTree::AgentRef* EveKDdroneManagementTree::CompareNodeToChildren( AgentRef* node )
{
	if( node == nullptr )
	{
		return nullptr;
	}

	if( node->left == nullptr && node->right == nullptr )
	{
		return node;
	}

	switch( node->planeType )
	{
	case X:
		if( IsBiggestOnAxis( node->left, node->agent->position.x, X ) && IsSmallestOnAxis( node->right, node->agent->position.x, X ) )
		{
			node->left = CompareNodeToChildren( node->left );
			node->right = CompareNodeToChildren( node->right );
			return node;
		}
		else
		{
			return SplitSort( node->b, node->e, X );
		}
		break;
	case Y:
		if( IsBiggestOnAxis( node->left, node->agent->position.y, Y ) && IsSmallestOnAxis( node->right, node->agent->position.y, Y ) )
		{
			node->left = CompareNodeToChildren( node->left );
			node->right = CompareNodeToChildren( node->right );
			return node;
		}
		else
		{
			return SplitSort( node->b, node->e, Y );
		}
		break;
	case Z:
		if( IsBiggestOnAxis( node->left, node->agent->position.z, Z ) && IsSmallestOnAxis( node->right, node->agent->position.z, Z ) )
		{
			node->left = CompareNodeToChildren( node->left );
			node->right = CompareNodeToChildren( node->right );
			return node;
		}
		else
		{
			return SplitSort( node->b, node->e, Z );
		}
		break;
	}
	return node;
}

bool EveKDdroneManagementTree::IsBiggestOnAxis( AgentRef* node, float n, PlaneType pt )
{
	if( node == nullptr )
	{
		return true;
	}

	switch( node->planeType )
	{
	case X:
		// if this is an X-split axis we only look at the bigger side
		if( pt == X )
		{
			return n >= node->agent->position.x && IsBiggestOnAxis( node->right, n, X );
		}
		else
		{
			return n >= node->agent->position.x && IsBiggestOnAxis( node->left, n, X ) && IsBiggestOnAxis( node->right, n, X );
		}
		break;
	case Y:
		if( pt == Y )
		{
			return n >= node->agent->position.y && IsBiggestOnAxis( node->right, n, Y );
		}
		else
		{
			return n >= node->agent->position.y && IsBiggestOnAxis( node->left, n, Y ) && IsBiggestOnAxis( node->right, n, Y );
		}
		break;
	case Z:
		if( pt == Z )
		{
			return n >= node->agent->position.z && IsBiggestOnAxis( node->right, n, Z );
		}
		else
		{
			return n >= node->agent->position.z && IsBiggestOnAxis( node->left, n, Z ) && IsBiggestOnAxis( node->right, n, Z );
		}
		break;
	}
	return true;
}


bool EveKDdroneManagementTree::IsSmallestOnAxis( AgentRef* node, const float n, PlaneType pt )
{
	if( node == nullptr )
	{
		return true;
	}

	switch( node->planeType )
	{
	case X:
		// if this is an X-split axis we only look at the smaller side
		if( pt == X )
		{
			return n <= node->agent->position.x && IsSmallestOnAxis( node->left, n, X );
		}
		else
		{
			return n <= node->agent->position.x && IsSmallestOnAxis( node->left, n, X ) && IsSmallestOnAxis( node->right, n, X );
		}
		break;
	case Y:
		if( pt == Y )
		{
			return n <= node->agent->position.y && IsSmallestOnAxis( node->left, n, Y );
		}
		else
		{
			return n <= node->agent->position.y && IsSmallestOnAxis( node->left, n, Y ) && IsSmallestOnAxis( node->right, n, Y );
		}
		break;
	case Z:
		if( pt == Z )
		{
			return n <= node->agent->position.z && IsSmallestOnAxis( node->left, n, Z );
		}
		else
		{
			return n <= node->agent->position.z && IsSmallestOnAxis( node->left, n, Z ) && IsSmallestOnAxis( node->right, n, Z );
		}
		break;
	}
	return true;
}

// agent = entire set, b = beginning of range, e = end of range, pt = what plane it should sort by
EveKDdroneManagementTree::AgentRef* EveKDdroneManagementTree::SplitSort( int b, int e, PlaneType pt )
{
	if( b == e )
	{
		m_agentRefs[b].b = b;
		m_agentRefs[e].e = e;
		m_agentRefs[b].left = nullptr;
		m_agentRefs[b].right = nullptr;
		return &m_agentRefs[b];
	}

	if( b > e )
	{
		return nullptr;
	}

	SortByAxis( m_agentRefs, b, e + 1, pt );

	int m = b + ( ( e - b ) / 2 ); // middlePoint

	m_agentRefs[m].b = b;
	m_agentRefs[m].e = e;

	pt = FindNextSplitAxis( pt );
	m_agentRefs[m].left = SplitSort( b, m - 1, pt );
	m_agentRefs[m].right = SplitSort( m + 1, e, pt );

	return &m_agentRefs[m];
}

void EveKDdroneManagementTree::ChangeAgentsIntoAgentRefs( std::vector<DroneAgent>& agents )
{
	m_agentRefs.clear();
	m_agentRefs.reserve( agents.size() );
	for( auto ag = agents.begin(); ag != agents.end(); ++ag )
	{
		AgentRef newRef;
		newRef.agent = &( *ag );
		m_agentRefs.push_back( newRef );
	}
}

EveKDdroneManagementTree::PlaneType EveKDdroneManagementTree::FindNextSplitAxis( PlaneType pt )
{
	switch( pt )
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

std::vector<EveKDdroneManagementTree::AgentRef>& EveKDdroneManagementTree::SortByAxis( std::vector<AgentRef>& agents, int b, int e, PlaneType pt ) const
{
	for( auto ag = agents.begin() + b; ag != agents.begin() + e; ++ag )
	{
		ag->planeType = pt;
	}

	std::sort( agents.begin() + b, agents.begin() + e, compareRef() );

	return agents;
}

// a simple function to handle when things outside of the current BehaviorGroup want to interact with
// or find a closest agent to a point. The group search ( FindDronesInRange() ) is more optimised / specialiced
// but this one is a standard tree search
DroneAgent* EveKDdroneManagementTree::FindClosestAgent( const Vector3 pos )
{
	if( m_tree.agent == nullptr )
	{
		return nullptr;
	}

	closestDrone drone;
	drone.agent = m_tree.agent;
	drone.rangeBetween = Length( m_tree.agent->position - pos );

	FindClosestAgentRecursive( pos, &m_tree, drone );
	return drone.agent;
}

void EveKDdroneManagementTree::FindClosestAgentRecursive( const Vector3& pos, AgentRef* currentNode, closestDrone& agent ) const
{
	if( currentNode == nullptr )
	{
		return;
	}

	float distToPoint = Length( currentNode->agent->position - pos );

	if( agent.rangeBetween > distToPoint )
	{
		agent.rangeBetween = distToPoint;
		agent.agent = currentNode->agent;
	}

	// Dig Through tree disregarding spaces on the other side of the splitting hyperplane
	switch( currentNode->planeType )
	{
	case X:
		if( currentNode->agent->position.x < pos.x )
		{
			FindClosestAgentRecursive( pos, currentNode->right, agent );

			// now when going back up through the recursion we have a best range to compare to
			if( currentNode->agent->position.x + agent.rangeBetween > pos.x )
			{
				FindClosestAgentRecursive( pos, currentNode->left, agent );
			}
		}
		else
		{
			FindClosestAgentRecursive( pos, currentNode->left, agent );

			if( currentNode->agent->position.x - agent.rangeBetween < pos.x )
			{
				FindClosestAgentRecursive( pos, currentNode->right, agent );
			}
		}
		break;
	case Y:
		if( currentNode->agent->position.y < pos.y )
		{
			FindClosestAgentRecursive( pos, currentNode->right, agent );

			if( currentNode->agent->position.y + agent.rangeBetween > pos.y )
			{
				FindClosestAgentRecursive( pos, currentNode->left, agent );
			}
		}
		else
		{
			FindClosestAgentRecursive( pos, currentNode->left, agent );

			if( currentNode->agent->position.y - agent.rangeBetween < pos.y )
			{
				FindClosestAgentRecursive( pos, currentNode->right, agent );
			}
		}
		break;
	case Z:
		if( currentNode->agent->position.z < pos.z )
		{
			FindClosestAgentRecursive( pos, currentNode->right, agent );

			if( currentNode->agent->position.z + agent.rangeBetween > pos.z )
			{
				FindClosestAgentRecursive( pos, currentNode->left, agent );
			}
		}
		else
		{
			FindClosestAgentRecursive( pos, currentNode->left, agent );

			if( currentNode->agent->position.z - agent.rangeBetween < pos.z )
			{
				FindClosestAgentRecursive( pos, currentNode->right, agent );
			}
		}
		break;
	}
}


// This is a very specialized function optimized for searching  for multiple ranges for multiple agents at the same time
// Use:		vvv = FindDronesInRange( (list of agents), (list of visionRanges/search-radiuses), (their own collision size / bounding sphere ));
// After:	vvv is an orginized tri-dementional list where the 1st index is the Behavior's index,
//			2nd the agent's index and the 3rd is the list of found agents in range (up to m_maxFoundPerAgent)
const std::vector<std::vector<std::vector<DroneAgent*>>>* EveKDdroneManagementTree::FindDronesInRange( std::vector<DroneAgent>& agents,
																									   std::vector<float>& ranges,
																									   const float& BehaviorGroupBoundingSphereRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<SearchRange> searchRanges;
	int behaviorNumber = 0;
	for( auto r = ranges.begin(); r != ranges.end(); ++r )
	{
		SearchRange br;
		br.behaviorNbr = behaviorNumber;
		behaviorNumber++;
		if( *r == -1 )
		{
			br.radius = -1;
		}
		else
		{
			br.radius = *r + BehaviorGroupBoundingSphereRadius;
		}
		searchRanges.push_back( br );
	}

	std::sort( searchRanges.begin(), searchRanges.end(), compareRef() );


	if( !m_groupSearchReturnInfoBlock.empty() )
	{
		for( int j = 0; j < behaviorNumber; ++j )
		{
			for( unsigned int i = 0; i < agents.size(); ++i )
			{
				m_groupSearchReturnInfoBlock[j][i].clear();
			}
		}
	}

	if( searchRanges.empty() )
	{
		return &m_groupSearchReturnInfoBlock;
	}

	if( searchRanges.begin()->radius == -1 )
	{
		return &m_groupSearchReturnInfoBlock;
	}

	int activeRange = 0;
	SearchThroughTree( m_groupSearchReturnInfoBlock, &m_tree, agents, searchRanges, activeRange );

	return &m_groupSearchReturnInfoBlock;
}

void EveKDdroneManagementTree::SearchThroughTree( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, std::vector<DroneAgent>& agents, const std::vector<SearchRange>& ranges, int& activeRange ) const
{
	int c = 0;
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++c )
	{
		activeRange = 0;
		SearchThroughTreeHelperFunction( closeAgents, node, *agent, ranges, activeRange, c );
	}
}

// this is a per agent helper function
void EveKDdroneManagementTree::SearchThroughTreeHelperFunction( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents,
																AgentRef* node,
																DroneAgent& agent,
																const std::vector<SearchRange>& ranges,
																int& activeRange,
																int& c ) const
{
	if( node == nullptr )
	{
		return;
	}

	if( activeRange > ( static_cast<int>( ranges.size() ) - 1 ) )
	{
		return;
	}

	if( ranges[activeRange].radius == -1 )
	{
		return;
	}

	float dist = LengthSq( node->agent->position - agent.position );
	float range = ranges[activeRange].radius;

	if( dist < range * range )
	{
		AddAgentToSearchLists( closeAgents, node, dist, ranges, activeRange, c );

		if( closeAgents[ranges[activeRange].behaviorNbr][c].size() < m_maxFoundPerAgent )
		{
			AddAgentToSearchLists( closeAgents, node, dist, ranges, activeRange, c );
		}
		else if( closeAgents[ranges[activeRange].behaviorNbr][c].size() == m_maxFoundPerAgent )
		{
			AddAgentToSearchLists( closeAgents, node, dist, ranges, activeRange, c );
			activeRange++;
		}
	}

	// switch here to check if pos + range cuts into axis splitter then search both, else just one;
	switch( node->planeType )
	{
	case X:
		if( node->agent->position.x - agent.position.x <= range )
		{
			SearchThroughTreeHelperFunction( closeAgents, node->right, agent, ranges, activeRange, c );
		}
		if( node->agent->position.x - agent.position.x >= range )
		{
			SearchThroughTreeHelperFunction( closeAgents, node->left, agent, ranges, activeRange, c );
		}
		break;
	case Y:
		if( node->agent->position.y - agent.position.y <= range )
		{
			SearchThroughTreeHelperFunction( closeAgents, node->right, agent, ranges, activeRange, c );
		}
		if( node->agent->position.y - agent.position.y >= range )
		{
			SearchThroughTreeHelperFunction( closeAgents, node->left, agent, ranges, activeRange, c );
		}
		break;
	case Z:
		if( node->agent->position.z - agent.position.z <= range )
		{
			SearchThroughTreeHelperFunction( closeAgents, node->right, agent, ranges, activeRange, c );
		}
		if( node->agent->position.z - agent.position.z >= range )
		{
			SearchThroughTreeHelperFunction( closeAgents, node->left, agent, ranges, activeRange, c );
		}
		break;
	}
}

void EveKDdroneManagementTree::AddAgentToSearchLists( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, float dist, const std::vector<SearchRange>& ranges, int activeRange, int agentNbr )
{
	for( auto r = ranges.begin() + activeRange; r != ranges.end(); ++r )
	{
		if( dist < r->radius * r->radius )
		{
			closeAgents[r->behaviorNbr][agentNbr].push_back( node->agent );
		}
		else
		{
			break;
		}
	}
}

void EveKDdroneManagementTree::RenderDebugInfo( ITr2DebugRenderer2& renderer, Matrix& parentWorldLocation )
{
	Vector3 debugSquareCorner1 = Vector3( m_debugSquareSize, m_debugSquareSize, m_debugSquareSize );
	Vector3 debugSquareCorner2 = debugSquareCorner1 * -1;

	renderer.DrawBox( nullptr, debugSquareCorner1, debugSquareCorner2, Tr2DebugRenderer::Wireframe, 0xff555555 );
	Vector3 pwt = parentWorldLocation.GetTranslation();
	DrawDebugTree( renderer, &m_tree, debugSquareCorner1, debugSquareCorner2, pwt );
}

void EveKDdroneManagementTree::DrawDebugTree( ITr2DebugRenderer2& renderer, AgentRef* tree, Vector3& debugSquareCorner1, Vector3& debugSquareCorner2, Vector3& pwt )
{
	if( tree == nullptr )
	{
		return;
	}

	if( tree->left == nullptr && tree->right == nullptr )
	{
		return;
	}

	Vector3 newCorner1;
	Vector3 newCorner2;

	switch( ( tree )->planeType )
	{
	case X:
		newCorner1 = Vector3( ( tree )->agent->position.x + .1f, debugSquareCorner1.y, debugSquareCorner1.z );
		newCorner2 = Vector3( ( tree )->agent->position.x - .1f, debugSquareCorner2.y, debugSquareCorner2.z );
		renderer.DrawBox( nullptr, newCorner1 + pwt, newCorner2 + pwt, Tr2DebugRenderer::Wireframe, 0xff2222ff );
		DrawSquareInnerLines( renderer, tree->agent->position, newCorner1, newCorner2, 0xff2222ff, X, pwt );
		DrawDebugTree( renderer, tree->left, newCorner1, debugSquareCorner2, pwt );
		DrawDebugTree( renderer, tree->right, debugSquareCorner1, newCorner2, pwt );
		break;
	case Y:
		newCorner1 = Vector3( debugSquareCorner1.x, ( tree )->agent->position.y + .1f, debugSquareCorner1.z );
		newCorner2 = Vector3( debugSquareCorner2.x, ( tree )->agent->position.y - .1f, debugSquareCorner2.z );
		renderer.DrawBox( nullptr, newCorner1 + pwt, newCorner2 + pwt, Tr2DebugRenderer::Wireframe, 0xff22ff22 );
		DrawSquareInnerLines( renderer, tree->agent->position, newCorner1, newCorner2, 0xff22ff22, Y, pwt );
		DrawDebugTree( renderer, tree->left, newCorner1, debugSquareCorner2, pwt );
		DrawDebugTree( renderer, tree->right, debugSquareCorner1, newCorner2, pwt );
		break;
	case Z:
		newCorner1 = Vector3( debugSquareCorner1.x, debugSquareCorner1.y, ( tree )->agent->position.z + .1f );
		newCorner2 = Vector3( debugSquareCorner2.x, debugSquareCorner2.y, ( tree )->agent->position.z - .1f );
		renderer.DrawBox( nullptr, newCorner1 + pwt, newCorner2 + pwt, Tr2DebugRenderer::Wireframe, 0xffff2222 );
		DrawSquareInnerLines( renderer, tree->agent->position, newCorner1, newCorner2, 0xffff2222, Z, pwt );
		DrawDebugTree( renderer, tree->left, newCorner1, debugSquareCorner2, pwt );
		DrawDebugTree( renderer, tree->right, debugSquareCorner1, newCorner2, pwt );
		break;
	}
	m_debugSquareSize = max( max( max( m_debugSquareSize, 1.5f * abs( tree->agent->position.x ) ),
								  1.5f * abs( tree->agent->position.y ) ),
							 1.5f * abs( tree->agent->position.z ) );
}

void EveKDdroneManagementTree::DrawSquareInnerLines( ITr2DebugRenderer2& renderer, Vector3& agentPos, Vector3& P1, Vector3& P2, Color C, PlaneType pt, Vector3& pwt )
{

	switch( pt )
	{
	case X:
		renderer.DrawLine( nullptr, agentPos + pwt, P1 + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, P2 + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, Vector3( agentPos.x, P1.y, P2.z ) + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, Vector3( agentPos.x, P2.y, P1.z ) + pwt, C );
		break;
	case Y:
		renderer.DrawLine( nullptr, agentPos + pwt, P1 + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, P2 + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, Vector3( P1.x, agentPos.y, P2.z ) + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, Vector3( P2.x, agentPos.y, P1.z ) + pwt, C );
		break;
	case Z:
		renderer.DrawLine( nullptr, agentPos + pwt, P1 + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, P2 + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, Vector3( P1.x, P2.y, agentPos.z ) + pwt, C );
		renderer.DrawLine( nullptr, agentPos + pwt, Vector3( P2.x, P1.y, agentPos.z ) + pwt, C );
		break;
	}
}

// expand file to review