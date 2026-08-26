// Copyright © 2026 CCP ehf.

#pragma once
#ifndef BVH_H
#define BVH_H

#include "../Resources/Tr2CmfContent.h"
#include "ITr2DebugRenderer2.h"
#include "GeometryUtils.h"

namespace BVH
{

const int32_t BVH_MAX_NODE_SIZE = 4;

struct alignas( 16 ) Node
{
	Vector3 boundsMin;
	uint32_t firstChildIndex : 26;
	uint32_t numObj : 3;
	uint32_t leaf : 1;
	uint32_t magicalPadding : 2; // this exists to prevent performance degradation due to simd interpreting its .w component as denormal float
	Vector3 boundsMax;
	uint32_t padding;

	Node() :
		boundsMin( {} ),
		firstChildIndex( 0 ),
		numObj( 0 ),
		leaf( 0 ),
		magicalPadding( 3 ),
		boundsMax( {} ),
		padding( 0 )
	{
	}
};

struct alignas( 16 ) Triangle
{
	Vector3 vertex0;
	uint32_t element : 30;
	uint32_t magicalPadding : 2; // this exists to prevent performance degradation due to simd interpreting its .w component as denormal float
	Vector3 edge1;
	uint32_t padding0;
	Vector3 edge2;
	uint32_t padding1;

	Triangle() :
		vertex0( {} ),
		element( 0 ),
		magicalPadding( 3 ),
		edge1( {} ),
		padding0( 0 ),
		edge2( {} ),
		padding1( 0 )
	{
	}
};

// Node and Triangle data layout have been optimized for SIMD. That's why there is padding in those structs, and why we assert it here.
static_assert( sizeof( Node ) == 32 );
static_assert( offsetof( Node, boundsMax ) == 16 );
static_assert( sizeof( Triangle ) == 48 );
static_assert( offsetof( Triangle, edge1 ) == 16 );

struct Tree
{
	std::vector<Triangle> triangles;
	std::vector<Node> nodes;
};

struct IntersectedNode
{
	const Node* node;
	float distance;
};

struct RayHit
{
	uint32_t meshIndex;
	uint32_t primitive;
	float u;
	float v;
	float distance;
};

class BoundingVolumeHierarchy
{
public:
	BoundingVolumeHierarchy() = default;
	BoundingVolumeHierarchy( Tr2CmfContents&& content, const std::vector<int32_t>& lodIndex );

	bool IntersectArea(
		std::vector<IntersectedNode>& stack,
		const CcpMath::Ray& ray,
		float rayLength,
		int32_t meshIndex,
		int32_t areaIndex,
		RayHit& result ) const;

	bool IntersectMesh(
		std::vector<IntersectedNode>& stack,
		const CcpMath::Ray& ray,
		float rayLength,
		int32_t meshIndex,
		RayHit& result ) const;

	bool IntersectAll(
		std::vector<IntersectedNode>& stack,
		const CcpMath::Ray& ray,
		float rayLength,
		RayHit& result ) const;

	bool IntersectAreaAcrossMeshes(
		std::vector<IntersectedNode>& stack,
		const CcpMath::Ray& ray,
		float rayLength,
		uint32_t areaIndex,
		RayHit& result ) const;

	cmf::ConstIndexBufferStream GetIndices( int meshIndex );
	cmf::ConstBufferElementStream<Vector3> GetPositions( int meshIndex );
	std::optional<cmf::ConstBufferElementStream<std::array<uint32_t, 4>>> GetBones( int meshIndex );
	std::optional<cmf::ConstBufferElementStream<Vector4>> GetColors( int meshIndex );

	void Visualize( Tr2DebugObjectReference owner, const Matrix& transform, ITr2DebugRenderer2& renderer ) const;

private:
	Tr2CmfContents m_content;
	std::vector<int32_t> m_lodIndices;
	std::vector<Tree> m_areaTrees;
	std::vector<uint32_t> m_areaOffsets;
};

}

#endif // BVH_H