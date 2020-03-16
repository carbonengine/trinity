#pragma once
#ifndef BoundingBox_H
#define BoundingBox_H

BLUE_DECLARE( TriViewport );

// --------------------------------------------------------------------------------------
// Description:
//   Class that represents an axis-aligned bounding box in 3D space.
// --------------------------------------------------------------------------------------
struct AxisAlignedBoundingBox
{
	AxisAlignedBoundingBox();
	AxisAlignedBoundingBox( const Vector3& min, const Vector3& max );
	AxisAlignedBoundingBox( const Vector4& sphere );

	void IncludePoint( const Vector3& pos );
	void IncludeBox( const AxisAlignedBoundingBox& other );

	bool IsPointInside( const Vector3& pos ) const;
	bool IsPointInside( const Vector3& pos, float epsilon ) const;
	bool Intersects( const AxisAlignedBoundingBox& other ) const;

	void Transform( const Matrix& transform );

	Vector3 m_min;
	Vector3 m_max;
};

bool BlueExtractArgumentImpl( BlueScriptArguments argument, AxisAlignedBoundingBox& result, unsigned int argID, std::false_type isBlueType );
BlueScriptValue BlueWrapReturnValueImpl( BlueScriptArguments args, const AxisAlignedBoundingBox& val );


// aa box
void BoundingBoxInitialize( Vector3& min, Vector3& max );
void BoundingBoxInitialize( const Vector4& sphere, Vector3& min, Vector3& max );

bool BoundingBoxIsInside( const Vector3& min, const Vector3& max, const Vector3& pos );
bool BoundingBoxIsInside( const Vector3& min, const Vector3& max, const Vector3& pos, float epsilon );

void BoundingBoxUpdate( Vector3& min, Vector3& max, const Vector3& pos );
void BoundingBoxUpdate( Vector3& min, Vector3& max, const Vector3& otherMin, const Vector3& otherMax );
void BoundingBoxUpdate( Vector3& min, Vector3& max, const Vector4& sphere );

// Transforms an axis aligned bounding box by the given transform, returns
// the new axis aligned bounding box
void BoundingBoxTransform( Vector3& min, Vector3& max, const Matrix& tf );

// Projects an axis aligned bounding box into screen space with the given view
// and projection matrices, along with a viewport.
void BoundingBoxProject( Vector3& min, Vector3& max, const Matrix& proj, const Matrix& view, const TriViewport& vp );

bool IntersectAxisAlignedBoxAxisAlignedBox( const Vector3& minBoundsA, const Vector3& maxBoundsA,
										    const Vector3& minBoundsB, const Vector3& maxBoundsB );
bool IntersectOrientedBoxAxisAlignedBox( const Vector3& centerA, const Vector3& extentsA, const Quaternion& orientationA, 
										 const Vector3& minBounds, const Vector3& maxBounds );
bool IntersectOrientedBoxOrientedBox( const Vector3& centerA, const Vector3& extentsA, const Quaternion& orientationA,
									  const Vector3& centerB, const Vector3& extentsB, const Quaternion& orientationB );

bool IntersectAxisAlignedBoxRay( const Vector3& minBounds, const Vector3& maxBounds, const Vector3& rayOrigin, const Vector3& rayDir, Vector3& intersection );

bool IntersectTriangleOrientedBox(const Vector3* triangleVertices, const Matrix& invOrientedBox );

bool IntersectTriangleOrientedBox( const Vector3* v0, 
								   const Vector3* v1, 
								   const Vector3* v2, 
								   const Matrix& invOrientedBox );

bool IntersectTriangleAABB( const Vector3* v0, 
							const Vector3* v1, 
							const Vector3* v2, 
							const Vector3& min, 
							const Vector3& max );

bool IsBoundingBoxEmpty( const Vector3& min, const Vector3& max );

#endif // BoundingBox_H