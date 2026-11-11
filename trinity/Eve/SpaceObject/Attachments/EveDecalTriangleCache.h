// Copyright © 2026 CCP ehf.

#pragma once

#include "../../../Resources/TriGeometryRes.h"
#include <atomic>

/**
 * @brief CPU copy of a mesh's triangles, bucketed in a hierarchical loose grid per LOD, so that a runtime
 * decal only tests the triangles near its volume instead of reading back and scanning the whole mesh.
 *
 * Each triangle is stored once, in the cell of its centre on the finest grid level whose cells are at
 * least as large as the triangle; a query looks at every level with its box grown by half a cell there.
 *
 * The first runtime decal build against a mesh reads the mesh back from the GPU on the main thread and
 * builds the grid on a worker thread; until that finishes, Get returns null and the decal tries again on a
 * later frame. The cache is owned by the mesh's TriGeometryResMeshData,
 * so it is released together with the geometry (TriGeometryRes::ReleaseResources clears the mesh data on
 * release, re-initialization and reload). It is also rebuilt if the mesh's LOD set no longer matches.
 */
struct DecalTriangleCache
{
	struct Level
	{
		Vector3 origin = Vector3( 0.f, 0.f, 0.f );
		Vector3 cellSize = Vector3( 0.f, 0.f, 0.f );
		Vector3 invCellSize = Vector3( 0.f, 0.f, 0.f );
		uint32_t dims[3] = { 1, 1, 1 };
		// index of this level's first cell in Lod::cellStart
		uint32_t firstCell = 0;
	};

	struct Lod
	{
		std::vector<Vector3> positions;
		// 3 per triangle, the LOD's index buffer values, reordered so each cell's triangles are contiguous
		std::vector<uint32_t> indices;
		std::vector<Level> levels;
		// triangles cellStart[c] .. cellStart[c + 1] (in units of triangles) belong to cell c
		std::vector<uint32_t> cellStart;
	};

	uint32_t lodMask = 0;
	std::vector<Lod> lods;
	// set by the worker thread once lods can be read
	std::atomic<bool> ready = false;

	/**
	 * @brief Returns the cache for meshData, starting to build it if needed.
	 * @return null while the mesh's buffers or the cache are not ready yet.
	 */
	static const DecalTriangleCache* Get( TriGeometryResMeshData* meshData );

	/**
	 * @brief Appends the indices of the triangles of LOD lodIndex that intersect the decal volume.
	 * @param aabbMin, aabbMax Bounds of the decal volume in the mesh's vertex space.
	 * @param inverseDecalMatrix Maps the mesh's vertex space into decal space (the volume is [-1, 1]^3).
	 * @param projectionAxis Unit direction the decal projects along, in the mesh's vertex space.
	 * @param minFacing Triangles whose face normal is closer to perpendicular to projectionAxis than
	 *   this (|cos| of the angle between them) are left out: the decal's texture would smear along
	 *   them. Either winding counts. Pass a value <= 0 to keep every triangle.
	 */
	void Query( size_t lodIndex, const Vector3& aabbMin, const Vector3& aabbMax, const Matrix& inverseDecalMatrix, const Vector3& projectionAxis, float minFacing, std::vector<uint32_t>& out ) const;

	/**
	 * @brief Finds the point on a hull edge of LOD lodIndex nearest to point, within radius.
	 *
	 * An edge is where two triangles meet at a fold sharper than sharpCos (the cosine of the
	 * angle between their face normals), or an open edge with a triangle on one side only.
	 * Vertices are matched by position, so the split vertices of a hard edge still join up.
	 * @param outNormal The two faces' normals averaged (a face's own on an open edge): a
	 *   decal projected along it wraps over the edge onto both faces.
	 * @return false if no edge lies within radius.
	 */
	bool FindNearestEdge( size_t lodIndex, const Vector3& point, float radius, float sharpCos, Vector3& outPosition, Vector3& outNormal, float& outDistance ) const;

	size_t GetMemoryUsage() const;
};
