#pragma once
#ifndef BoundingBox_H
#define BoundingBox_H

BLUE_DECLARE( TriViewport );


using AxisAlignedBoundingBox = CcpMath::AxisAlignedBox;

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

Vector3 ClosestPointToBoundingBox( const Vector3& min, const Vector3& max, const Vector3& point );


template <typename Iterator>
void CreateItemSetBoundingBoxes( CcpMath::AxisAlignedBox& staticBounds, std::vector<std::pair<int, CcpMath::AxisAlignedBox>>& boneBounds, bool skinned, Iterator itemsBegin, Iterator itemsEnd )
{
	boneBounds.clear();
	staticBounds = CcpMath::AxisAlignedBox();

	std::map<int32_t, CcpMath::AxisAlignedBox> boxes;

	for( auto it = itemsBegin; it != itemsEnd; ++it )
	{
		auto item = *it;
		auto itemBounds = item->GetBounds();
		auto boneIndex = item->GetBoneIndex();
		if( skinned && boneIndex >= 0 )
		{
			auto found = boxes.find( boneIndex );
			if( found != end( boxes ) )
			{
				found->second.Include( itemBounds );
			}
			else
			{
				boxes[boneIndex] = CcpMath::AxisAlignedBox( itemBounds );
			}
		}
		else
		{
			// Group together all static items not attached to any bone
			staticBounds.Include( itemBounds );
		}
	}
	boneBounds.insert( end( boneBounds ), begin( boxes ), end( boxes ) );
}

CcpMath::AxisAlignedBox GetItemSetAabb( const CcpMath::AxisAlignedBox& staticBounds, const std::vector<std::pair<int, CcpMath::AxisAlignedBox>>& boneBounds, const granny_matrix_3x4* bones, size_t boneCount );

#endif // BoundingBox_H