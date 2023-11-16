#include "StdAfx.h"

#include "Obb.h"
#include "TriFrustum.h"

// -------------------------------------------------------------
// Description:
//   Computes an Oriented Bounding Box in world space, which has been shrunk
//   to fit the current viewing frustum as tightly as possible without visibly clipping
//   into it.  The frustum is derived from Tr2Renderer GetViewTransform, GetProjectionTransform, and so on.
//   The calculation starts out with the AABB from GetLocalBoundingBox, so the explicit bounds
//   are supported.
// Arguments:
//   localMin     - minimum of AABB in local coordinates
//   localMax     - maximum of AABB in local coordinates
//   localToWorld - [in]  transform that takes this skinned object from local to world coordinates
//   frustum      - frustum to shrink against. If null, no shrinking occurs and this is just a convenience setup helper (AABB->OBB)
// Summary:
//   Compute a world-space OBB, tightened for the current frustum.
// -------------------------------------------------------------
void Obb::CreateClippedWorldBoundingObb( const Vector3& min, const Vector3& max, const Matrix& localToWorld, const TriFrustum* frustum )
{
	// take AABB center..
	center = 0.5 * ( max + min );

	// .. and move to world space.
	Vector4 centerWorld = Transform( center, localToWorld );
	center = Vector3( centerWorld.x, centerWorld.y, centerWorld.z );


	sizes  = 0.5 * ( max - min );
	x      = Vector3( localToWorld._11, localToWorld._12, localToWorld._13 );
	y      = Vector3( localToWorld._21, localToWorld._22, localToWorld._23 );
	z      = Vector3( localToWorld._31, localToWorld._32, localToWorld._33 );

	if( !frustum )
	{
		return;
	}

	const float largeF = 9e29f;

	// Shrink this OBB against the frustum, basically by raycasting along its edges to move each of the six sides
	// just enough to make the corner points touch a frustum plane -- if the entire plane was outside the frustum, that is.

	// For the six sides of the OBB, figure out the 4 corner points;
	// sequence: -X, +X, -Y, +Y, -Z, +Z
	for( unsigned side = 0; side != 6; ++side )
	{
		unsigned pt[4];		// bitmasks of points wanted, bits are 00000ZYX, not set = negative, set = positive side.
		Vector3 rayDir;		// direction of the ray to raycast with; ie the direction along which we try to move an OBB side

		switch( side )
		{
			// -X, so bit 0 zero
			case 0: 
				pt[0] = 0; pt[1] = 2; pt[2] = 4; pt[3] = 6;
				rayDir = x;
				break;

			// +X, so bit 0 set
			case 1: 
				pt[0] = 1; pt[1] = 3; pt[2] = 5; pt[3] = 7;
				rayDir = -x;
				break;

			// -Y, so bit 1 zero
			case 2: 
				pt[0] = 0; pt[1] = 1; pt[2] = 4; pt[3] = 5;
				rayDir = y;
				break;

			// +Y, so bit 1 set
			case 3: 
				pt[0] = 2; pt[1] = 3; pt[2] = 6; pt[3] = 7;
				rayDir = -y;
				break;

			// -Z, so bit 2 zero
			case 4: 
				pt[0] = 0; pt[1] = 1; pt[2] = 3; pt[3] = 4;
				rayDir = z;
				break;

			// +Z, so bit 2 set
			case 5: 
				pt[0] = 4; pt[1] = 5; pt[2] = 6; pt[3] = 7;
				rayDir = -z;
				break;
		}

		float movement = largeF;
		
		for( unsigned frside = 0; frside != 6; ++frside )
		{
			const Plane& plane = frustum->m_planes[ frside ];

			float planeMove = largeF;
			bool pointInside = false;	// leave OBB side alone if any of the 4 points is already inside the frustum.
			for( unsigned i = 0; i != 4 && !pointInside; ++i )
			{
				Vector3 p;

				p = center;
				p += ( ( pt[i] & 1 ) ? sizes.x : -sizes.x ) * x;
				p += ( ( pt[i] & 2 ) ? sizes.y : -sizes.y ) * y;
				p += ( ( pt[i] & 4 ) ? sizes.z : -sizes.z ) * z;

				// if all points are below the plane, then figure out the shortest distance
				// we can move them along rayDir without actually making them visible.
				// By then taking the minimum of this movement over all possible planes, we
				// move the points to the frustum without any visible clipping of the OBB.
				
				const float distance = DotCoord( plane, p );
				if( distance >= 0 )
				{
					// if any part of the square is visible, don't move at all.
					pointInside = true;
					continue;
				}

				const float vn = DotNormal( plane, rayDir );
				if( vn < 0.0001f )	// parallel, or negative (= OBB side points towards frustum plane's outside)
				{
					continue;
				}

				float t = - distance / vn;
				if( t <= 0 )
				{
					continue;
				}

				planeMove = std::min( planeMove, t );
			}

			if( !pointInside && planeMove < largeF )
			{
				movement = std::min( movement, planeMove );
			}
		}

		if( movement < largeF )
		{
			// move the OBB side by distance 'movement' along rayDir.
			// Which means moving the center by half that and shrinking the OBB by half that, so the
			// other end remains in place.
			movement /= 2;
			center += movement * rayDir;

			switch( side )
			{
				case 0 : 
				case 1 : sizes.x -= movement; break;

				case 2 : 
				case 3 : sizes.y -= movement; break;

				case 4 : 
				case 5 : sizes.z -= movement; break;
			}
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Return a corner point of the OBB.
//	 Iterated in order -x/+x, -y/+y, -z/+z.
// Arguments:
//   N - point, range 0..7
// -------------------------------------------------------------
Vector3 Obb::GetPoint( unsigned N ) const
{
	static const float X[8] = { 1.0f, -1.0f,  1.0f, -1.0f,  1.0f, -1.0f,  1.0f, -1.0f };
	static const float Y[8] = { 1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f };
	static const float Z[8] = { 1.0f,  1.0f,  1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f };

	CCP_ASSERT( N < 8 );
	return center + X[N] * sizes[0] * x + Y[N] * sizes[1] * y + Z[N] * sizes[2] * z;
}

// -------------------------------------------------------------
// Description:
//   Find the AABB of the OBB after running it through the given transform.
// Arguments:
//   min - [out] minimum of AABB
//   max - [out] maximum of AABB
//   transform - [in] local to world transform
// -------------------------------------------------------------
void Obb::ComputeAABB( Vector3& min, Vector3& max, const Matrix& transform ) const
{
	Vector3 in = GetPoint( 0 );
	Vector3 out = TransformCoord( in, transform );

	min = max = out;
	for( unsigned i = 1; i != 8; ++i )
	{
		in = GetPoint( i );
		out = TransformCoord( in, transform );

		min.x = std::min( min.x, out.x );
		min.y = std::min( min.y, out.y );
		min.z = std::min( min.z, out.z );

		max.x = std::max( max.x, out.x );
		max.y = std::max( max.y, out.y );
		max.z = std::max( max.z, out.z );
	}
}
