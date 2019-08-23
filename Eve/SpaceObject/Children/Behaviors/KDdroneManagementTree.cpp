#include "StdAfx.h"
#include "KDdroneManagementTree.h"

KDdroneManagementTree::KDdroneManagementTree(IRoot* lockobj) :
	m_isInitialized( false ),
	m_debugSquareSize( 0 ),
	m_timeBetweenUpdate( 1 ), //update once per sec
	m_maxFoundPerAgent( 5 )
{
}

KDdroneManagementTree::~KDdroneManagementTree()
{
}

void KDdroneManagementTree::CreateTree(std::vector<DroneAgent>& agents)
{
	if( agents.empty() )
	{
		return;
	}

	ChangeAgentsIntoAgentRefs( agents );
	AgentRef tree = *SplitSort( 0, static_cast< int > ( agents.size() ) - 1, X);
	m_tree = tree;
	m_isInitialized = true;
}

void KDdroneManagementTree::UpdateTree( const float dt )
{
	if ( m_updateTimeCounter >= m_timeBetweenUpdate )
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
AgentRef* KDdroneManagementTree::CompareNodeToChildren( AgentRef* node )
{
	if ( node == nullptr )
	{
		return nullptr;
	}

	if ( node->Left == nullptr  && node->Right == nullptr )
	{
		return node;
	}

	switch ( node->planeType )
	{
	case X:
		if (IsBiggestOnAxis(node->Left, node->agent->position.x, X) && IsSmallestOnAxis(
			node->Right, node->agent->position.x, X))
		{
			node->Left = CompareNodeToChildren( node->Left );
			node->Right = CompareNodeToChildren( node->Right );
			return node;
		}
		else
		{
			return SplitSort( node->b, node->e, X );
		}
		break;
	case Y:
		if ( IsBiggestOnAxis( node->Left, node->agent->position.y, Y ) && IsSmallestOnAxis(
			node->Right, node->agent->position.y, Y ) )
		{
			node->Left = CompareNodeToChildren( node->Left );
			node->Right = CompareNodeToChildren( node->Right );
			return node;
		}
		else
		{
			return SplitSort( node->b, node->e, Y );
		}
		break;
	case Z:
		if ( IsBiggestOnAxis( node->Left, node->agent->position.z, Z ) && IsSmallestOnAxis(
			node->Right, node->agent->position.z, Z ) )
		{
			node->Left = CompareNodeToChildren( node->Left );
			node->Right = CompareNodeToChildren( node->Right );
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

bool KDdroneManagementTree::IsBiggestOnAxis( AgentRef* node, float n, planeType pt )
{
	if ( node == nullptr )
	{
		return true;
	}

	switch ( node->planeType )
	{
	case X:
		// if this is an X-split axis we only look at the bigger side
		if(node->planeType == X)
		{
			return n >= node->agent->position.x && IsBiggestOnAxis( node->Right, n, X );
		}
		else
		{
			return n >= node->agent->position.x && IsBiggestOnAxis( node->Left, n, X ) 
				&& IsBiggestOnAxis( node->Right, n, X );
		}
		break;
	case Y:
		if ( node->planeType == Y )
		{
			return n >= node->agent->position.y && IsBiggestOnAxis( node->Right, n, Y );
		}
		else
		{
			return n >= node->agent->position.y && IsBiggestOnAxis( node->Left, n, Y )
				&& IsBiggestOnAxis( node->Right, n, Y );
		}
		break;
	case Z:
		if ( node->planeType == Z )
		{
			return n >= node->agent->position.z && IsBiggestOnAxis( node->Right, n, Z );
		}
		else
		{
			return n >= node->agent->position.z && IsBiggestOnAxis( node->Left, n, Z )
				&& IsBiggestOnAxis( node->Right, n, Z );
		}
		break;
	}
	return true;
}


bool KDdroneManagementTree::IsSmallestOnAxis( AgentRef* node, float n, planeType pt )
{
	if ( node == nullptr )
	{
		return true;
	}

	switch ( node->planeType )
	{
	case X:
		// if this is an X-split axis we only look at the smaller side
		if ( node->planeType == X )
		{
			return n <= node->agent->position.x && IsSmallestOnAxis( node->Left, n, X );
		}
		else
		{
			return n <= node->agent->position.x && IsSmallestOnAxis( node->Left, n, X )
				&& IsSmallestOnAxis( node->Right, n, X );
		}
		break;
	case Y:
		if ( node->planeType == Y )
		{
			return n <= node->agent->position.y && IsSmallestOnAxis( node->Left, n, Y );
		}
		else
		{
			return n <= node->agent->position.y && IsSmallestOnAxis( node->Left, n, Y )
				&& IsSmallestOnAxis( node->Right, n, Y );
		}
		break;
	case Z:
		if ( node->planeType == Z )
		{
			return n <= node->agent->position.z && IsSmallestOnAxis( node->Left, n, Z );
		}
		else
		{
			return n <= node->agent->position.z && IsSmallestOnAxis( node->Left, n, Z )
				&& IsSmallestOnAxis( node->Right, n, Z );
		}
		break;
	}
	return true;
}

// agent = entire set, b = beginning of range, e = end of range, pt = what plane it should sort by
AgentRef* KDdroneManagementTree::SplitSort( int b, int e, planeType pt )
{
	if( b == e )
	{
		m_agentRefs[b].b = b;
		m_agentRefs[b].e = e;
		m_agentRefs[ b ].Left = nullptr;
		m_agentRefs[ b ].Right = nullptr;
		return &m_agentRefs[b];
	}

	if( b > e )
	{
		return nullptr;
	}

	sortByAxis( m_agentRefs, b, e + 1, pt );

	int m = b + static_cast< int >(floor( (e - b) / 2.f )); // middlePoint

	m_agentRefs[ m ].b = b;
	m_agentRefs[ m ].e = e;

	pt = FindNextSplitAxis( pt );
	m_agentRefs[ m ].Left = SplitSort( b, m - 1,pt);
	m_agentRefs[ m ].Right = SplitSort( m + 1, e, pt);

	return &m_agentRefs[m];
}

void KDdroneManagementTree::ChangeAgentsIntoAgentRefs( std::vector<DroneAgent>& agents )
{
	
	for ( auto ag = agents.begin(); ag != agents.end(); ++ag )
	{
		AgentRef newRef;
		newRef.agent = &(*ag);
		m_agentRefs.push_back( newRef );
	}
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

std::vector<AgentRef>& KDdroneManagementTree::sortByAxis( std::vector<AgentRef>& agents, int b, int e, planeType pt ) const
{
	for ( auto ag = agents.begin() + b; ag != agents.begin() + e; ++ag )
	{
		ag->planeType = pt;
	}

	std::sort( agents.begin() + b, agents.begin() + e, compareRef() );

	return agents;
}

DroneAgent* KDdroneManagementTree::findClosestAgent( Vector3 pos )
{
	// a simple function to handle when things outside of the current BehaviorGroup want to interact with
	// or find a closest agent to a point. The group search ( FindDronesInRange() ) is more optimised / specialiced
	// but this one is a standard tree search

	if ( m_tree.agent == nullptr ) 
	{
		return nullptr;
	}

	closestDrone drone;
	drone.agent = m_tree.agent;
	drone.rangeBetweenSq = Length( m_tree.agent->position - pos );

	findClosestAgentRecursive( pos, &m_tree, drone );
	return drone.agent;
}

void KDdroneManagementTree::findClosestAgentRecursive( Vector3& pos, AgentRef* currentNode, closestDrone& agent )
{
	if ( currentNode == nullptr )
	{
		return;
	}

	float distToPoint = Length( currentNode->agent->position - pos );
	
	if ( agent.rangeBetweenSq > distToPoint )
	{
		agent.rangeBetweenSq = distToPoint;
		agent.agent = currentNode->agent;
	}

	// Dig Through tree disregarding spaces on the other side of the splitting hyperplane
	switch ( currentNode->planeType )
	{
	case X:
		if ( currentNode->agent->position.x < pos.x )
		{
			findClosestAgentRecursive( pos, currentNode->Right, agent );

			// now when going back up through the recursion we have a best range to compare to
			if ( currentNode->agent->position.x + agent.rangeBetweenSq > pos.x )
			{
				findClosestAgentRecursive( pos, currentNode->Left, agent );
			}
		}
		else
		{
			findClosestAgentRecursive( pos, currentNode->Left, agent );

			if ( currentNode->agent->position.x - agent.rangeBetweenSq < pos.x )
			{
				findClosestAgentRecursive( pos, currentNode->Right, agent );
			}
		}
		break;
	case Y:
		if ( currentNode->agent->position.y < pos.y )
		{
			findClosestAgentRecursive( pos, currentNode->Right, agent );

			if ( currentNode->agent->position.y + agent.rangeBetweenSq > pos.y )
			{
				findClosestAgentRecursive( pos, currentNode->Left, agent );
			}
		}
		else
		{
			findClosestAgentRecursive( pos, currentNode->Left, agent );

			if ( currentNode->agent->position.y - agent.rangeBetweenSq < pos.y )
			{
				findClosestAgentRecursive( pos, currentNode->Right, agent );
			}
		}
		break;
	case Z:
		if ( currentNode->agent->position.z < pos.z )
		{
			findClosestAgentRecursive( pos, currentNode->Right, agent );

			if ( currentNode->agent->position.z + agent.rangeBetweenSq > pos.z )
			{
				findClosestAgentRecursive( pos, currentNode->Left, agent );
			}
		}
		else
		{
			findClosestAgentRecursive( pos, currentNode->Left, agent );

			if ( currentNode->agent->position.z - agent.rangeBetweenSq < pos.z )
			{
				findClosestAgentRecursive( pos, currentNode->Right, agent );
			}
		}
		break;
	}
}


// This is a very specialized function optimized for searching  for multiple ranges for multiple agents at the same time
// Use: vvv = FindDronesInRange( (list of agents), (list of visionRanges/search-radiuses), (their own collision size / bounding sphere ));
// After: vvv is an orginized tri-dementional list where the 1st index is the Behavior's index, 2nd the agent's index and the 3rd is the list of found agents in range (up to m_maxFoundPerAgent) 
std::vector<std::vector<std::vector<DroneAgent*>>> KDdroneManagementTree::FindDronesInRange( std::vector<DroneAgent>& agents, std::vector<float>& ranges, float& BehaviorGroupboundingSphereRadius )
{
	std::vector<searchRange> searchRanges;
	int BNbr = 0;
	for ( auto r = ranges.begin(); r != ranges.end(); ++r )
	{
		searchRange br;
		br.behaviorNbr = BNbr;
		BNbr++;
		if ( *r == -1 )
		{
			br.radius = -1;
		}
		else
		{
			br.radius = *r + BehaviorGroupboundingSphereRadius;
		}
		searchRanges.push_back( br );
	}

	std::sort( searchRanges.begin(), searchRanges.end(), compareRef() );

	std::vector< std::vector < std::vector <DroneAgent*>>> returnInfoBlock;

	for ( int j = 0; j < BNbr; j++ )
	{
		std::vector < std::vector <DroneAgent*>> perAgentData;
		for ( unsigned int i = 0; i < agents.size(); i++ )
		{
			perAgentData.push_back( std::vector <DroneAgent*>() );
		}
		returnInfoBlock.push_back( perAgentData );
	}

	if ( searchRanges.empty() )
	{
		return returnInfoBlock;
	}

	if ( searchRanges.begin()->radius == -1 )
	{
		return returnInfoBlock;
	}

	int activeRange = 0;
	searchThroughTree( returnInfoBlock, &m_tree, agents, searchRanges, activeRange );

	return returnInfoBlock;
}

void KDdroneManagementTree::searchThroughTree( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, std::vector<DroneAgent>& agents, std::vector<searchRange>& ranges, int& activeRange ) const
{
	int c = 0;
	for ( auto agent = agents.begin(); agent != agents.end(); ++agent, c++ )
	{
		activeRange = 0;
		searchThroughTreeHelperFunction( closeAgents, node, *agent, ranges, activeRange, c );
	}
}

// this is a per agent helper function
void KDdroneManagementTree::searchThroughTreeHelperFunction( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, DroneAgent& agent, std::vector<searchRange>& ranges, int& activeRange, int& c ) const
{
	if ( node == nullptr )
	{
		return;
	}

	if ( ranges[ activeRange ].radius == -1 )
	{
		return;
	}

	float dist = LengthSq( node->agent->position - agent.position );
	float range = ranges[ activeRange ].radius;

	if ( dist < range * range )
	{
		if ( activeRange > ( static_cast< int >( ranges.size() ) - 1 ) )
		{
			return;
		}

		AddAgentToSearchLists( closeAgents, node, dist, ranges, activeRange, c );

		if ( closeAgents[ ranges[ activeRange ].behaviorNbr ][ c ].size() < m_maxFoundPerAgent )
		{
			AddAgentToSearchLists( closeAgents, node, dist, ranges, activeRange, c );
		}
		else if ( closeAgents[ ranges[ activeRange ].behaviorNbr ][ c ].size() == m_maxFoundPerAgent )
		{
			AddAgentToSearchLists( closeAgents, node, dist, ranges, activeRange, c );
			activeRange++;
		}
	}

	// switch here to check if pos + range cuts into axis splitter then search both, else just one;
	switch ( node->planeType )
	{
	case X:
		if ( node->agent->position.x - agent.position.x < 0 + range )
		{
			searchThroughTreeHelperFunction( closeAgents, node->Right, agent, ranges, activeRange, c );
		}
		if ( node->agent->position.x - agent.position.x > 0 + range )
		{
			searchThroughTreeHelperFunction( closeAgents, node->Left, agent, ranges, activeRange, c );
		}
		break;
	case Y:
		if ( node->agent->position.y - agent.position.y < 0 + range )
		{
			searchThroughTreeHelperFunction( closeAgents, node->Right, agent, ranges, activeRange, c );
		}
		if ( node->agent->position.y - agent.position.y > 0 + range )
		{
			searchThroughTreeHelperFunction( closeAgents, node->Left, agent, ranges, activeRange, c );
		}
		break;
	case Z:
		if ( node->agent->position.z - agent.position.z < 0 + range )
		{
			searchThroughTreeHelperFunction( closeAgents, node->Right, agent, ranges, activeRange, c );
		}
		if ( node->agent->position.z - agent.position.z > 0 + range )
		{
			searchThroughTreeHelperFunction( closeAgents, node->Left, agent, ranges, activeRange, c );
		}
		break;
	}
}

void KDdroneManagementTree::AddAgentToSearchLists( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, float& dist, std::vector<searchRange>& ranges, int& activeRange, int& agentNbr )
{
	for ( auto r = ranges.begin() + activeRange; r != ranges.end(); ++r )
	{
		if( dist < r->radius * r->radius )
		{
			closeAgents[ r->behaviorNbr ][agentNbr].push_back( node->agent );
		}
		else
		{
			break;
		}
	}
}

void KDdroneManagementTree::RenderDebugInfo( Tr2DebugRenderer& renderer, Matrix& parentWorldLocation )
{
	Vector3 debugSquareCorner1 = Vector3( m_debugSquareSize, m_debugSquareSize, m_debugSquareSize );
	Vector3 debugSquareCorner2 = debugSquareCorner1 * -1;

	renderer.DrawBox( nullptr, debugSquareCorner1, debugSquareCorner2, Tr2DebugRenderer::Wireframe, 0xff555555 );
	Vector3 pwt = parentWorldLocation.GetTranslation();
	DrawTree( renderer, &m_tree, debugSquareCorner1, debugSquareCorner2, pwt );
}

void KDdroneManagementTree::DrawTree( Tr2DebugRenderer& renderer, AgentRef* tree,  Vector3& debugSquareCorner1, Vector3& debugSquareCorner2, Vector3& pwt )
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
		newCorner1 = Vector3( ( tree )->agent->position.x + .1f, debugSquareCorner1.y, debugSquareCorner1.z );
		newCorner2 = Vector3( ( tree )->agent->position.x - .1f, debugSquareCorner2.y, debugSquareCorner2.z );
		renderer.DrawBox( nullptr, newCorner1 + pwt, newCorner2 + pwt, Tr2DebugRenderer::Wireframe, 0xffff2222 );
		DrawSquareInnerLines( renderer, tree->agent->position, newCorner1, newCorner2, 0xffff2222, X, pwt );
		DrawTree( renderer, tree->Left, debugSquareCorner2, newCorner1, pwt );
		DrawTree( renderer, tree->Right, newCorner2, debugSquareCorner1, pwt );
		break;
	case Y:
		newCorner1 = Vector3( debugSquareCorner1.x, ( tree )->agent->position.y + .1f, debugSquareCorner1.z );
		newCorner2 = Vector3( debugSquareCorner2.x, ( tree )->agent->position.y - .1f, debugSquareCorner2.z );
		renderer.DrawBox( nullptr, newCorner1 + pwt, newCorner2 + pwt, Tr2DebugRenderer::Wireframe, 0xff22ff22 );
		DrawSquareInnerLines( renderer, tree->agent->position, newCorner1, newCorner2, 0xff22ff22, Y, pwt );
		DrawTree( renderer, ( *tree ).Left, newCorner2, debugSquareCorner1, pwt );
		DrawTree( renderer, ( *tree ).Right, debugSquareCorner2, newCorner1, pwt );
		break;
	case Z:
		newCorner1 = Vector3( debugSquareCorner1.x, debugSquareCorner1.y, ( tree )->agent->position.z + .1f );
		newCorner2 = Vector3( debugSquareCorner2.x, debugSquareCorner2.y, ( tree )->agent->position.z - .1f );
		renderer.DrawBox( nullptr, newCorner1 + pwt, newCorner2 + pwt, Tr2DebugRenderer::Wireframe, 0xff2222ff );
		DrawSquareInnerLines( renderer, tree->agent->position, newCorner1, newCorner2, 0xff2222ff, Z, pwt );
		DrawTree( renderer, ( *tree ).Left, debugSquareCorner2, newCorner1, pwt );
		DrawTree( renderer, ( *tree ).Right, newCorner2, debugSquareCorner1, pwt );
		break;
	}
	m_debugSquareSize = max(max(max(m_debugSquareSize, 1.5f * abs(tree->agent->position.x) ), 1.5f * abs(tree->agent->position.y) ),
		1.5f * abs(tree->agent->position.z) );
}

void KDdroneManagementTree::DrawSquareInnerLines( Tr2DebugRenderer& renderer, Vector3& agentPos, Vector3& P1, Vector3& P2, Color C, planeType pt, Vector3& pwt )
{
	
	switch ( pt )
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