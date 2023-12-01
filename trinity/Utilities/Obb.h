#pragma once
#ifndef OBB_H
#define OBB_H

class TriFrustum;

// --------------------------------------------------------------------------------------
// Description:
//   Minimal implementation of oriented bounding boxes. The code in NCC that optimizes
//   the shadow frustum for the camera position can be useful in other places, and also
//   being able to pass around OBBs and fitting a frustum around them instead of losing
//   space due to multiple AABB approximations, is better.  So that code got moved here
//   to enable sharing
// See Also:
//   Vector3
// --------------------------------------------------------------------------------------
struct Obb
{
	// c'tor left blank for speed

	//   x, y, z      - normalized vectors defining the OBB's orientation
	Vector3 x;
	Vector3 y;
	Vector3 z;
	
	//   center       - the center, in world coordinates, of the OBB
	Vector3 center;

	//   sizes        - half the size of the OBB along every axis x, y or z. Ie you get to a corner point with center + sizes[0] * x. Full width/height/depth is sizes*2.
	Vector3 sizes;

	void CreateClippedWorldBoundingObb( const Vector3& localMin, const Vector3& localMax, const Matrix& localToWorld, const TriFrustum* frustum );

	Vector3 GetPoint( unsigned N ) const;

	void ComputeAABB( Vector3& min, Vector3& max, const Matrix& transform ) const;
};

#endif // OBB_H