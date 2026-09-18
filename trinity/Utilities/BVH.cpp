// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "BVH.h"

namespace BVH
{

struct Primitive
{
	uint32_t element;
	CcpMath::AxisAlignedBox aabb;
};

int FindLargestDimension( Vector3 dimensions )
{
	if( dimensions.x > dimensions.y )
	{
		if( dimensions.x > dimensions.z )
		{
			return 0;
		}
		else
		{
			return 2;
		}
	}
	else
	{
		if( dimensions.y > dimensions.z )
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
}

CcpMath::AxisAlignedBox CreateAABB( const std::vector<Primitive>& primitives, int from, int to )
{
	CcpMath::AxisAlignedBox aabb{};
	for( int i = from; i < to; i++ )
	{
		aabb.Include( primitives[i].aabb );
	}
	return aabb;
}

void CreateNodes(
	std::vector<Primitive>& primitives,
	std::vector<Node>& nodes,
	int leftIndex,
	int rightIndex,
	uint32_t nodeIndex )
{
	Node& node = nodes[nodeIndex];
	if( rightIndex - leftIndex > BVH_MAX_NODE_SIZE )
	{
		int dimension = FindLargestDimension( node.boundsMax - node.boundsMin );

		// If we were to move the bvh construction into cmf, we could choose more expensive splitting criteria. Right now this has to be fast.
		float target = ( node.boundsMax[dimension] + node.boundsMin[dimension] );
		auto partitionedElement = std::partition( primitives.begin() + leftIndex, primitives.begin() + rightIndex, [dimension, target]( const Primitive& p ) {
			return ( p.aabb.m_max[dimension] + p.aabb.m_min[dimension] ) < target;
		} );

		int split = (int)std::distance( primitives.begin(), partitionedElement );
		if( split <= leftIndex || split >= rightIndex )
		{
			split = ( rightIndex + leftIndex ) >> 1;
			std::nth_element( primitives.begin() + leftIndex, primitives.begin() + split, primitives.begin() + rightIndex, [dimension]( const Primitive& a, const Primitive& b ) {
				return ( a.aabb.m_max[dimension] + a.aabb.m_min[dimension] ) < ( b.aabb.m_max[dimension] + b.aabb.m_min[dimension] );
			} );
		}

		node.firstChildIndex = nodes.size();
		node.numObj = 2;
		node.leaf = false;
		Node leftChild;
		auto leftChildAABB = CreateAABB( primitives, leftIndex, split );
		leftChild.boundsMin = leftChildAABB.m_min;
		leftChild.boundsMax = leftChildAABB.m_max;
		Node rightChild;
		auto rightChildAABB = CreateAABB( primitives, split, rightIndex );
		rightChild.boundsMin = rightChildAABB.m_min;
		rightChild.boundsMax = rightChildAABB.m_max;
		nodes.push_back( leftChild );
		nodes.push_back( rightChild );
		uint32_t leftChildIndex = (uint32_t)nodes.size() - 2;
		uint32_t rightChildIndex = (uint32_t)nodes.size() - 1;
		CreateNodes( primitives, nodes, leftIndex, split, leftChildIndex );
		CreateNodes( primitives, nodes, split, rightIndex, rightChildIndex );
	}
	else
	{
		node.firstChildIndex = leftIndex;
		node.numObj = rightIndex - leftIndex;
		node.leaf = true;
	}
}

Tree CreateTree(
	const cmf::ConstIndexBufferStream& indices,
	const cmf::ConstBufferElementStream<Vector3>& positions,
	uint32_t firstElement,
	uint32_t elementCount )
{
	Tree tree{};

	if( elementCount == 0 )
	{
		return tree;
	}

	std::vector<Primitive> primitives;
	primitives.reserve( elementCount );
	tree.triangles.reserve( elementCount );
	tree.nodes.reserve( 2 * elementCount - 1 );

	auto triangleVertices = [&indices, &positions]( int32_t i, Vector3 vertices[3] ) {
		int index0 = indices[i * 3];
		int index1 = indices[i * 3 + 1];
		int index2 = indices[i * 3 + 2];

		vertices[0] = positions[index0];
		vertices[1] = positions[index1];
		vertices[2] = positions[index2];
	};

	for( uint32_t i = firstElement; i < firstElement + elementCount; i++ )
	{
		Primitive primitive;

		Vector3 vertices[3];
		triangleVertices( i, vertices );
		primitive.aabb.IncludePoint( vertices[0] );
		primitive.aabb.IncludePoint( vertices[1] );
		primitive.aabb.IncludePoint( vertices[2] );

		primitive.element = i;

		primitives.push_back( primitive );
	}

	int leftIndex = 0;
	int rightIndex = elementCount;
	Node root;
	auto rootAABB = CreateAABB( primitives, 0, (int)primitives.size() );
	root.boundsMin = rootAABB.m_min;
	root.boundsMax = rootAABB.m_max;
	tree.nodes.push_back( root );
	CreateNodes( primitives, tree.nodes, leftIndex, rightIndex, 0 );

	for( int i = 0; i < primitives.size(); i++ )
	{
		Triangle triangle;
		Vector3 vertices[3];
		triangleVertices( primitives[i].element, vertices );
		triangle.vertex0 = vertices[0];
		triangle.edge1 = vertices[1] - vertices[0];
		triangle.edge2 = vertices[2] - vertices[0];
		triangle.element = primitives[i].element;
		tree.triangles.push_back( triangle );
	}

	tree.nodes.shrink_to_fit();

	return tree;
}

// modified version of IntersectAxisAlignedBoxRay
bool Intersects( const XMVECTOR& origin, const XMVECTOR& invRayDir, const Node& node, float& distance )
{
	XMVECTOR minA = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &node.boundsMin ) );
	XMVECTOR maxA = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &node.boundsMax ) );

	XMVECTOR t0 = ( minA - origin ) * invRayDir;
	XMVECTOR t1 = ( maxA - origin ) * invRayDir;

	XMVECTOR smallerIntersection = XMVectorMin( t0, t1 );
	XMVECTOR biggerIntersection = XMVectorMax( t0, t1 );

	float minT = max( XMVectorGetX( smallerIntersection ), max( XMVectorGetY( smallerIntersection ), XMVectorGetZ( smallerIntersection ) ) );
	float maxT = min( XMVectorGetX( biggerIntersection ), min( XMVectorGetY( biggerIntersection ), XMVectorGetZ( biggerIntersection ) ) );

	distance = minT;

	return maxT > 0.f && maxT >= minT;
}

bool Intersection(
	const Tree& tree,
	std::vector<IntersectedNode>& stack,
	const CcpMath::Ray& ray,
	float rayLength,
	uint32_t& primitive,
	float& u,
	float& v,
	float& distance )
{
	if( tree.nodes.empty() )
	{
		return false;
	}

	XMVECTOR rayOrigin = ray.origin;
	XMVECTOR rayDir = ray.direction;
	XMVECTOR invRayDir = XMVectorReciprocal( ray.direction );
	invRayDir = XMVectorClamp(
		invRayDir,
		XMVectorReplicate( -std::numeric_limits<float>::max() ),
		XMVectorReplicate( std::numeric_limits<float>::max() ) );

	float hitDistance;
	if( !Intersects( rayOrigin, invRayDir, tree.nodes[0], hitDistance ) || rayLength < hitDistance )
	{
		return false;
	}

	stack.clear();
	bool hit = false;
	const Node* currentNode = &tree.nodes[0];
	while( true )
	{
		if( !currentNode->leaf )
		{
			const Node* leftChild = &tree.nodes[currentNode->firstChildIndex];
			const Node* rightChild = &tree.nodes[currentNode->firstChildIndex + 1];
			float leftDistance;
			float rightDistance;
			bool hitLeft = Intersects( rayOrigin, invRayDir, *leftChild, leftDistance ) && leftDistance <= rayLength;
			bool hitRight = Intersects( rayOrigin, invRayDir, *rightChild, rightDistance ) && rightDistance <= rayLength;
			if( hitLeft && hitRight )
			{
				if( leftDistance < rightDistance )
				{
					stack.push_back( IntersectedNode{ rightChild, rightDistance } );
					currentNode = leftChild;
				}
				else
				{
					stack.push_back( IntersectedNode{ leftChild, leftDistance } );
					currentNode = rightChild;
				}
				continue;
			}
			else if( hitLeft )
			{
				currentNode = leftChild;
				continue;
			}
			else if( hitRight )
			{
				currentNode = rightChild;
				continue;
			}
		}
		else
		{
			for( uint32_t i = currentNode->firstChildIndex; i < currentNode->firstChildIndex + currentNode->numObj; i++ )
			{
				float hitU, hitV;
				XMVECTOR vertex0 = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &tree.triangles[i].vertex0 ) );
				XMVECTOR edge1 = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &tree.triangles[i].edge1 ) );
				XMVECTOR edge2 = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &tree.triangles[i].edge2 ) );
				if( IntersectTriXM( vertex0, edge1, edge2, rayOrigin, rayDir, hitU, hitV, hitDistance ) )
				{
					if( hitDistance < rayLength )
					{
						distance = rayLength = hitDistance;
						u = hitU;
						v = hitV;
						primitive = tree.triangles[i].element;
						hit = true;
					}
				}
			}
		}
		while( stack.size() > 0 && stack.back().distance > rayLength )
		{
			stack.pop_back();
		}
		if( stack.size() > 0 )
		{
			currentNode = stack.back().node;
			stack.pop_back();
		}
		else
		{
			break;
		}
	}

	return hit;
}

const uint32_t INVALID_AREA_OFFSET = std::numeric_limits<uint32_t>::max();

BoundingVolumeHierarchy::BoundingVolumeHierarchy( Tr2CmfContents&& content, const std::vector<int32_t>& lodIndices )
{
	m_content = std::move( content );
	m_lodIndices = lodIndices;
	uint32_t areasOffset = 0;
	for( int32_t i = 0; i < m_content.GetData()->meshes.size(); i++ )
	{
		const auto& mesh = m_content.GetData()->meshes[i];

		if( mesh.topology != cmf::MeshTopology::TriangleList )
		{
			m_areaOffsets.push_back( INVALID_AREA_OFFSET );
			continue;
		}
		m_areaOffsets.push_back( areasOffset );
		areasOffset += uint32_t( mesh.areas.size() );

		auto indices = GetIndices( i );
		auto positions = GetPositions( i );

		for( const auto& area : mesh.lods[lodIndices[i]].areas )
		{
			m_areaTrees.push_back( CreateTree( indices, positions, area.firstElement, area.elementCount ) );
		}
	}
}

bool BoundingVolumeHierarchy::IntersectArea(
	std::vector<IntersectedNode>& stack,
	const CcpMath::Ray& ray,
	float rayLength,
	int32_t meshIndex,
	int32_t areaIndex,
	RayHit& result ) const
{
	uint32_t areasOffset = m_areaOffsets[meshIndex];
	if( areasOffset == INVALID_AREA_OFFSET )
	{
		return false;
	}

	bool hit = BVH::Intersection( m_areaTrees[areasOffset + areaIndex], stack, ray, rayLength, result.primitive, result.u, result.v, result.distance );
	if( hit )
	{
		result.meshIndex = meshIndex;
	}
	return hit;
}

bool BoundingVolumeHierarchy::IntersectMesh(
	std::vector<IntersectedNode>& stack,
	const CcpMath::Ray& ray,
	float rayLength,
	int32_t meshIndex,
	RayHit& result ) const
{
	uint32_t areasOffset = m_areaOffsets[meshIndex];
	if( areasOffset == INVALID_AREA_OFFSET )
	{
		return false;
	}

	bool hit = false;
	for( size_t areaIndex = 0; areaIndex < m_content.GetData()->meshes[meshIndex].areas.size(); areaIndex++ )
	{
		const auto& tree = m_areaTrees[areasOffset + areaIndex];
		if( BVH::Intersection( tree, stack, ray, rayLength, result.primitive, result.u, result.v, rayLength ) )
		{
			result.distance = rayLength;
			result.meshIndex = meshIndex;
			hit = true;
		}
	}

	return hit;
}

bool BoundingVolumeHierarchy::IntersectAll(
	std::vector<IntersectedNode>& stack,
	const CcpMath::Ray& ray,
	float rayLength,
	RayHit& result ) const
{
	bool hit = false;
	for( size_t i = 0; i < m_content.GetData()->meshes.size(); i++ )
	{
		uint32_t areasOffset = m_areaOffsets[i];
		if( areasOffset == INVALID_AREA_OFFSET )
		{
			continue;
		}

		const auto& mesh = m_content.GetData()->meshes[i];
		for( size_t areaIndex = 0; areaIndex < mesh.areas.size(); areaIndex++ )
		{
			const auto& tree = m_areaTrees[areasOffset + areaIndex];
			if( BVH::Intersection( tree, stack, ray, rayLength, result.primitive, result.u, result.v, rayLength ) )
			{
				result.meshIndex = (uint32_t)i;
				result.distance = rayLength;
				hit = true;
			}
		}
	}
	return hit;
}

bool BoundingVolumeHierarchy::IntersectAreaAcrossMeshes(
	std::vector<IntersectedNode>& stack,
	const CcpMath::Ray& ray,
	float rayLength,
	uint32_t areaIndex,
	RayHit& result ) const
{
	bool hit = false;
	for( size_t i = 0; i < m_content.GetData()->meshes.size(); i++ )
	{
		uint32_t areasOffset = m_areaOffsets[i];
		if( areasOffset == INVALID_AREA_OFFSET )
		{
			continue;
		}

		const auto& mesh = m_content.GetData()->meshes[i];
		if( areaIndex < mesh.areas.size() )
		{
			const auto& tree = m_areaTrees[areasOffset + areaIndex];
			if( BVH::Intersection( tree, stack, ray, rayLength, result.primitive, result.u, result.v, rayLength ) )
			{
				result.meshIndex = (uint32_t)i;
				result.distance = rayLength;
				hit = true;
			}
		}
	}
	return hit;
}

cmf::ConstIndexBufferStream BoundingVolumeHierarchy::GetIndices( int meshIndex )
{
	const auto& mesh = m_content.GetData()->meshes[meshIndex];
	int32_t lodIndex = m_lodIndices[meshIndex];
	auto ib = mesh.lods[lodIndex].ib;
	auto ibSectionData = m_content.GetSection( ib.index );
	auto indices = cmf::ConstIndexBufferStream( ibSectionData, ib );
	return indices;
}

cmf::ConstBufferElementStream<Vector3> BoundingVolumeHierarchy::GetPositions( int meshIndex )
{
	const auto& mesh = m_content.GetData()->meshes[meshIndex];
	int32_t lodIndex = m_lodIndices[meshIndex];
	auto positionElement = cmf::FindElement( mesh.decl, cmf::Usage::Position );
	auto vb = mesh.lods[lodIndex].vb;
	uint32_t numVerts = cmf::GetStreamElementCount( vb );
	auto vertices = m_content.GetViewData( vb );
	cmf::ConstBufferElementStream<Vector3> positions( *positionElement, vertices, numVerts, vb.stride );
	return positions;
}

std::optional<cmf::ConstBufferElementStream<std::array<uint32_t, 4>>> BoundingVolumeHierarchy::GetBones( int meshIndex )
{
	const auto& mesh = m_content.GetData()->meshes[meshIndex];
	int32_t lodIndex = m_lodIndices[meshIndex];
	auto boneElement = cmf::FindElement( mesh.decl, cmf::Usage::BoneIndices );
	auto vb = mesh.lods[lodIndex].vb;
	uint32_t numVerts = cmf::GetStreamElementCount( vb );
	auto vertices = m_content.GetViewData( vb );
	std::optional<cmf::ConstBufferElementStream<std::array<uint32_t, 4>>> bones;
	if( boneElement )
	{
		bones.emplace( *boneElement, vertices, numVerts, vb.stride );
	}
	return bones;
}

std::optional<cmf::ConstBufferElementStream<Vector4>> BoundingVolumeHierarchy::GetColors( int meshIndex )
{
	const auto& mesh = m_content.GetData()->meshes[meshIndex];
	int32_t lodIndex = m_lodIndices[meshIndex];
	auto colorElement = cmf::FindElement( mesh.decl, cmf::Usage::Color );
	auto vb = mesh.lods[lodIndex].vb;
	uint32_t numVerts = cmf::GetStreamElementCount( vb );
	auto vertices = m_content.GetViewData( vb );
	std::optional<cmf::ConstBufferElementStream<Vector4>> colors;
	if( colorElement )
	{
		colors.emplace( *colorElement, vertices, numVerts, vb.stride );
	}
	return colors;
}

void BoundingVolumeHierarchy::Visualize( Tr2DebugObjectReference owner, const Matrix& transform, ITr2DebugRenderer2& renderer ) const
{
	for( const auto& tree : m_areaTrees )
	{
		for( const auto& node : tree.nodes )
		{
			if( node.leaf )
			{
				renderer.DrawBox( owner, transform, node.boundsMin, node.boundsMax, ITr2DebugRenderer2::Wireframe, Color( 1.f, 0.f, 0.f, 1.f ) );
			}
			else
			{
				renderer.DrawBox( owner, transform, node.boundsMin, node.boundsMax, ITr2DebugRenderer2::Wireframe, Color( 1.f, 1.f, 1.f, 1.f ) );
			}
		}
	}
}

}
