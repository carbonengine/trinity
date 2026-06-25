// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "BoundingBox.h"
#include "BoundingSphere.h"
#include "MatrixUtils.h"

#include "TriViewport.h"

static const XMVECTOR g_UnitQuaternionEpsilon = {
	1.0e-4f,
	1.0e-4f,
	1.0e-4f,
	1.0e-4f
};

static inline bool XMVector3AnyTrue( FXMVECTOR V )
{
	XMVECTOR C;

	// Duplicate the fourth element from the first element.
	C = XMVectorSwizzle( V, 0, 1, 2, 0 );

	return XMComparisonAnyTrue( XMVector4EqualIntR( C, XMVectorTrueInt() ) );
}

static inline bool XMQuaternionIsUnit( FXMVECTOR Q )
{
	XMVECTOR Difference = XMVector4Length( Q ) - XMVectorSplatOne();

	return XMVector4Less( XMVectorAbs( Difference ), g_UnitQuaternionEpsilon ) != 0;
}


// --------------------------------------------------------------------------------------
// Description:
//   Helper function to convert python value to an AxisAlignedBoundingBox. Used by Blue
//   exposure functions. The AABB is represented as a sequence ((x,y,z),(x,y,z)) in Python.
// Arguments:
//   argument - Python function argument
//   result - Resulting AABB
//   argID - Argument number (used for error reporting)
//   isBlueType - This is not an IRoot descendant
// Return value:
//   true if convertion successful
//   false otherwise
// --------------------------------------------------------------------------------------
#if BLUE_WITH_PYTHON
bool BlueExtractArgumentImpl(
	PyObject* argument,
	AxisAlignedBoundingBox& result,
	unsigned int argID,
	std::false_type isBlueType )
{
	if( !PySequence_Check( argument ) )
	{
		PyErr_Format( PyExc_TypeError,
					  "Could not match argument %i to expected type: sequence of 2 3D vectors expected.",
					  argID );
		return false;
	}

	Py_ssize_t tupleCount = PySequence_Size( argument );
	if( tupleCount != 2 )
	{
		PyErr_Format( PyExc_TypeError,
					  "Could not match argument %i to expected type: sequence of 2 3D vectors expected.",
					  argID );
		return false;
	}

	PyObject* item = PySequence_ITEM( argument, 0 );
	bool success = BlueExtractVector( item, &result.m_min.x, 3 );
	Py_DecRef( item );
	if( !success )
	{
		PyErr_Format( PyExc_TypeError,
					  "Could not match argument %i to expected type: sequence of 2 3D vectors expected.",
					  argID );
		return false;
	}

	item = PySequence_ITEM( argument, 1 );
	success = BlueExtractVector( item, &result.m_max.x, 3 );
	Py_DecRef( item );
	if( !success )
	{
		PyErr_Format( PyExc_TypeError,
					  "Could not match argument %i to expected type: sequence of 2 3D vectors expected.",
					  argID );
		return false;
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to convert AxisAlignedBoundingBox to a python value. Used by Blue
//   exposure functions. The AABB is represented as a sequence ((x,y,z),(x,y,z)) in Python.
// Arguments:
//   val - AABB
// Return value:
//   Python representation of AABB
// --------------------------------------------------------------------------------------
BlueScriptValue BlueWrapReturnValueImpl( BlueScriptArguments args, const AxisAlignedBoundingBox& val )
{
	return Py_BuildValue(
		"(fff)(fff)",
		val.m_min.x,
		val.m_min.y,
		val.m_min.z,
		val.m_max.x,
		val.m_max.y,
		val.m_max.z );
}
#endif

void BoundingBoxInitialize( Vector3& min, Vector3& max )
{
	min.x = FLT_MAX;
	min.y = FLT_MAX;
	min.z = FLT_MAX;

	max.x = -FLT_MAX;
	max.y = -FLT_MAX;
	max.z = -FLT_MAX;
}

void BoundingBoxInitialize( const Vector4& sphere, Vector3& min, Vector3& max )
{
	min = Vector3( sphere.x - sphere.w, sphere.y - sphere.w, sphere.z - sphere.w );
	max = Vector3( sphere.x + sphere.w, sphere.y + sphere.w, sphere.z + sphere.w );
}

bool BoundingBoxIsInside( const Vector3& min, const Vector3& max, const Vector3& pos )
{
	if( ( min.x > pos.x ) || ( max.x < pos.x ) )
	{
		return false;
	}
	if( ( min.y > pos.y ) || ( max.y < pos.y ) )
	{
		return false;
	}
	if( ( min.z > pos.z ) || ( max.z < pos.z ) )
	{
		return false;
	}
	return true;
}

bool BoundingBoxIsInside( const Vector3& min, const Vector3& max, const Vector3& pos, float epsilon )
{
	if( ( min.x > pos.x + epsilon ) || ( max.x < pos.x - epsilon ) )
	{
		return false;
	}
	if( ( min.y > pos.y + epsilon ) || ( max.y < pos.y - epsilon ) )
	{
		return false;
	}
	if( ( min.z > pos.z + epsilon ) || ( max.z < pos.z - epsilon ) )
	{
		return false;
	}
	return true;
}

void BoundingBoxUpdate( Vector3& min, Vector3& max, const Vector3& otherMin, const Vector3& otherMax )
{
	BoundingBoxUpdate( min, max, otherMin );
	BoundingBoxUpdate( min, max, otherMax );
}

void BoundingBoxUpdate( Vector3& min, Vector3& max, const Vector3& pos )
{
	if( pos.x < min.x )
	{
		min.x = pos.x;
	}
	if( pos.x > max.x )
	{
		max.x = pos.x;
	}

	if( pos.y < min.y )
	{
		min.y = pos.y;
	}
	if( pos.y > max.y )
	{
		max.y = pos.y;
	}

	if( pos.z < min.z )
	{
		min.z = pos.z;
	}
	if( pos.z > max.z )
	{
		max.z = pos.z;
	}
}

void BoundingBoxUpdate( Vector3& min, Vector3& max, const Vector4& sphere )
{
	BoundingBoxUpdate( min, max, Vector3( sphere.x - sphere.w, sphere.y - sphere.w, sphere.z - sphere.w ) );
	BoundingBoxUpdate( min, max, Vector3( sphere.x + sphere.w, sphere.y + sphere.w, sphere.z + sphere.w ) );
}

void BoundingBoxTransform( Vector3& min, Vector3& max, const Matrix& tf )
{
	Vector3 corners[8];

	corners[0] = min;

	corners[1].x = min.x;
	corners[1].y = min.y;
	corners[1].z = max.z;

	corners[2].x = max.x;
	corners[2].y = min.y;
	corners[2].z = min.z;

	corners[3].x = max.x;
	corners[3].y = min.y;
	corners[3].z = max.z;

	corners[4] = max;

	corners[5].x = max.x;
	corners[5].y = max.y;
	corners[5].z = min.z;

	corners[6].x = min.x;
	corners[6].y = max.y;
	corners[6].z = max.z;

	corners[7].x = min.x;
	corners[7].y = max.y;
	corners[7].z = min.z;

	TransformCoords( corners, tf );

	min = corners[0];
	max = corners[0];
	float* pMin = (float*)&min;
	float* pMax = (float*)&max;

	for( int i = 1; i < 8; ++i )
	{
		float* pCorner = (float*)&corners[i];

		for( int component = 0; component < 3; ++component )
		{
			if( pCorner[component] < pMin[component] )
			{
				pMin[component] = pCorner[component];
			}
			if( pCorner[component] > pMax[component] )
			{
				pMax[component] = pCorner[component];
			}
		}
	}
}

void BoundingBoxProject( Vector3& min, Vector3& max, const Matrix& view, const Matrix& proj, const TriViewport& vp )
{
	Vector3 corners[8];

	corners[0] = min;

	corners[1].x = min.x;
	corners[1].y = min.y;
	corners[1].z = max.z;

	corners[2].x = max.x;
	corners[2].y = min.y;
	corners[2].z = min.z;

	corners[3].x = max.x;
	corners[3].y = min.y;
	corners[3].z = max.z;

	corners[4] = max;

	corners[5].x = max.x;
	corners[5].y = max.y;
	corners[5].z = min.z;

	corners[6].x = min.x;
	corners[6].y = max.y;
	corners[6].z = max.z;

	corners[7].x = min.x;
	corners[7].y = max.y;
	corners[7].z = min.z;

	Matrix viewProj = view * proj;
	for( int i = 0; i < 8; ++i )
	{
		corners[i] = TransformCoord( corners[i], viewProj );
		Vec3TransformByViewport( corners[i], vp );
	}

	min = corners[0];
	max = corners[0];
	float* pMin = (float*)&min;
	float* pMax = (float*)&max;

	for( int i = 1; i < 8; ++i )
	{
		float* pCorner = (float*)&corners[i];

		for( int component = 0; component < 3; ++component )
		{
			if( pCorner[component] < pMin[component] )
			{
				pMin[component] = pCorner[component];
			}
			if( pCorner[component] > pMax[component] )
			{
				pMax[component] = pCorner[component];
			}
		}
	}
}

bool IntersectAxisAlignedBoxAxisAlignedBox( const Vector3& minBoundsA, const Vector3& maxBoundsA, const Vector3& minBoundsB, const Vector3& maxBoundsB )
{
	XMVECTOR MinA = XMVectorSet( minBoundsA.x, minBoundsA.y, minBoundsA.z, 0.0f );
	XMVECTOR MaxA = XMVectorSet( maxBoundsA.x, maxBoundsA.y, maxBoundsA.z, 0.0f );

	XMVECTOR MinB = XMVectorSet( minBoundsB.x, minBoundsB.y, minBoundsB.z, 0.0f );
	XMVECTOR MaxB = XMVectorSet( maxBoundsB.x, maxBoundsB.y, maxBoundsB.z, 0.0f );

	// for each i in (x, y, z) if a_min(i) > b_max(i) or b_min(i) > a_max(i) then return FALSE
	XMVECTOR Disjoint = XMVectorOrInt( XMVectorGreater( MinA, MaxB ), XMVectorGreater( MinB, MaxA ) );

	return !XMVector3AnyTrue( Disjoint );
}


// --------------------------------------------------------------------------------------
// Description:
//   A ray AABB intersection check.
// Arguments:
//   minBounds, maxBounds - The min/max bounds of the AABB
//   rayOrigin, rayDir - The ray origin and direction (normalized)
//   intersection - The intersection point if there is any
// Return Value:
//   false if ray does not intersect AABB
//   true if ray may intersect AABB
// --------------------------------------------------------------------------------------
bool IntersectAxisAlignedBoxRay( const Vector3& minBounds, const Vector3& maxBounds, const Vector3& rayOrigin, const Vector3& rayDir, Vector3& intersection )
{
	if( minBounds == maxBounds )
	{
		// we are actually dealing with a point but not a box
		intersection = minBounds;
		return LengthSq( Normalize( minBounds - rayOrigin ) - rayDir ) < 1e10;
	}

	XMVECTOR minA = XMVectorSet( minBounds.x, minBounds.y, minBounds.z, 0.0f );
	XMVECTOR maxA = XMVectorSet( maxBounds.x, maxBounds.y, maxBounds.z, 0.0f );

	XMVECTOR invRayDir = XMVectorReciprocal( rayDir );

	XMVECTOR t0 = ( minA - rayOrigin ) * invRayDir;
	XMVECTOR t1 = ( maxA - rayOrigin ) * invRayDir;

	XMVECTOR smallerIntersection = XMVectorMin( t0, t1 );
	XMVECTOR biggerIntersection = XMVectorMax( t0, t1 );

	float minT = max( XMVectorGetX( smallerIntersection ), max( XMVectorGetY( smallerIntersection ), XMVectorGetZ( smallerIntersection ) ) );
	float maxT = min( XMVectorGetX( biggerIntersection ), min( XMVectorGetY( biggerIntersection ), XMVectorGetZ( biggerIntersection ) ) );

	intersection = rayOrigin + minT * rayDir;
	return minT < maxT;
}

bool IntersectOrientedBoxAxisAlignedBox( const Vector3& centerA, const Vector3& extentsA, const Quaternion& orientationA, const Vector3& minBounds, const Vector3& maxBounds )
{
	Vector3 centerB = 0.5f * ( maxBounds + minBounds );
	Vector3 extentsB = maxBounds - centerB;
	Quaternion orientationB( 0.0f, 0.0f, 0.0f, 1.0f );

	return IntersectOrientedBoxOrientedBox( centerA, extentsA, orientationA, centerB, extentsB, orientationB );
}

bool IntersectOrientedBoxOrientedBox( const Vector3& centerA, const Vector3& extentsA, const Quaternion& orientationA, const Vector3& centerB, const Vector3& extentsB, const Quaternion& orientationB )
{
#define Permute0W1Z0Y0X XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X
#define Permute0Z0W1X0Y XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y
#define Permute1Y0X0W0Z XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z
#define PermuteWZYX XM_PERMUTE_0W, XM_PERMUTE_0Z, XM_PERMUTE_0Y, XM_PERMUTE_0X
#define PermuteZWXY XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_0X, XM_PERMUTE_0Y
#define PermuteYXWZ XM_PERMUTE_0Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z

	// Build the 3x3 rotation matrix that defines the orientation of B relative to A.
	XMVECTOR A_quat = XMVectorSet( orientationA.x, orientationA.y, orientationA.z, orientationA.w );
	XMVECTOR B_quat = XMVectorSet( orientationB.x, orientationB.y, orientationB.z, orientationB.w );

	assert( XMQuaternionIsUnit( A_quat ) );
	assert( XMQuaternionIsUnit( B_quat ) );

	XMVECTOR Q = XMQuaternionMultiply( A_quat, XMQuaternionConjugate( B_quat ) );
	XMMATRIX R = XMMatrixRotationQuaternion( Q );

	// Compute the translation of B relative to A.
	XMVECTOR A_cent = XMVectorSet( centerA.x, centerA.y, centerA.z, 0.0f );
	XMVECTOR B_cent = XMVectorSet( centerB.x, centerB.y, centerB.z, 0.0f );
	XMVECTOR t = XMVector3InverseRotate( B_cent - A_cent, A_quat );

	//
	// h(A) = extents of A.
	// h(B) = extents of B.
	//
	// a(u) = axes of A = (1,0,0), (0,1,0), (0,0,1)
	// b(u) = axes of B relative to A = (r00,r10,r20), (r01,r11,r21), (r02,r12,r22)
	//
	// For each possible separating axis l:
	//   d(A) = sum (for i = u,v,w) h(A)(i) * abs( a(i) dot l )
	//   d(B) = sum (for i = u,v,w) h(B)(i) * abs( b(i) dot l )
	//   if abs( t dot l ) > d(A) + d(B) then disjoint
	//

	// Load extents of A and B.
	XMVECTOR h_A = XMVectorSet( extentsA.x, extentsA.y, extentsA.z, 0.0f );
	XMVECTOR h_B = XMVectorSet( extentsB.x, extentsB.y, extentsB.z, 0.0f );

	// Rows. Note R[0,1,2]X.w = 0.
	XMVECTOR R0X = R.r[0];
	XMVECTOR R1X = R.r[1];
	XMVECTOR R2X = R.r[2];

	R = XMMatrixTranspose( R );

	// Columns. Note RX[0,1,2].w = 0.
	XMVECTOR RX0 = R.r[0];
	XMVECTOR RX1 = R.r[1];
	XMVECTOR RX2 = R.r[2];

	// Absolute value of rows.
	XMVECTOR AR0X = XMVectorAbs( R0X );
	XMVECTOR AR1X = XMVectorAbs( R1X );
	XMVECTOR AR2X = XMVectorAbs( R2X );

	// Absolute value of columns.
	XMVECTOR ARX0 = XMVectorAbs( RX0 );
	XMVECTOR ARX1 = XMVectorAbs( RX1 );
	XMVECTOR ARX2 = XMVectorAbs( RX2 );

	// Test each of the 15 possible seperating axii.
	XMVECTOR d, d_A, d_B;

	// l = a(u) = (1, 0, 0)
	// t dot l = t.x
	// d(A) = h(A).x
	// d(B) = h(B) dot abs(r00, r01, r02)
	d = XMVectorSplatX( t );
	d_A = XMVectorSplatX( h_A );
	d_B = XMVector3Dot( h_B, AR0X );
	XMVECTOR NoIntersection = XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) );

	// l = a(v) = (0, 1, 0)
	// t dot l = t.y
	// d(A) = h(A).y
	// d(B) = h(B) dot abs(r10, r11, r12)
	d = XMVectorSplatY( t );
	d_A = XMVectorSplatY( h_A );
	d_B = XMVector3Dot( h_B, AR1X );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(w) = (0, 0, 1)
	// t dot l = t.z
	// d(A) = h(A).z
	// d(B) = h(B) dot abs(r20, r21, r22)
	d = XMVectorSplatZ( t );
	d_A = XMVectorSplatZ( h_A );
	d_B = XMVector3Dot( h_B, AR2X );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = b(u) = (r00, r10, r20)
	// d(A) = h(A) dot abs(r00, r10, r20)
	// d(B) = h(B).x
	d = XMVector3Dot( t, RX0 );
	d_A = XMVector3Dot( h_A, ARX0 );
	d_B = XMVectorSplatX( h_B );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = b(v) = (r01, r11, r21)
	// d(A) = h(A) dot abs(r01, r11, r21)
	// d(B) = h(B).y
	d = XMVector3Dot( t, RX1 );
	d_A = XMVector3Dot( h_A, ARX1 );
	d_B = XMVectorSplatY( h_B );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = b(w) = (r02, r12, r22)
	// d(A) = h(A) dot abs(r02, r12, r22)
	// d(B) = h(B).z
	d = XMVector3Dot( t, RX2 );
	d_A = XMVector3Dot( h_A, ARX2 );
	d_B = XMVectorSplatZ( h_B );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(u) x b(u) = (0, -r20, r10)
	// d(A) = h(A) dot abs(0, r20, r10)
	// d(B) = h(B) dot abs(0, r02, r01)
	d = XMVector3Dot( t, XMVectorPermute( RX0, -RX0, Permute0W1Z0Y0X ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX0, ARX0, PermuteWZYX ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR0X, AR0X, PermuteWZYX ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(u) x b(v) = (0, -r21, r11)
	// d(A) = h(A) dot abs(0, r21, r11)
	// d(B) = h(B) dot abs(r02, 0, r00)
	d = XMVector3Dot( t, XMVectorPermute( RX1, -RX1, Permute0W1Z0Y0X ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX1, ARX1, PermuteWZYX ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR0X, AR0X, PermuteZWXY ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(u) x b(w) = (0, -r22, r12)
	// d(A) = h(A) dot abs(0, r22, r12)
	// d(B) = h(B) dot abs(r01, r00, 0)
	d = XMVector3Dot( t, XMVectorPermute( RX2, -RX2, Permute0W1Z0Y0X ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX2, ARX2, PermuteWZYX ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR0X, AR0X, PermuteYXWZ ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(v) x b(u) = (r20, 0, -r00)
	// d(A) = h(A) dot abs(r20, 0, r00)
	// d(B) = h(B) dot abs(0, r12, r11)
	d = XMVector3Dot( t, XMVectorPermute( RX0, -RX0, Permute0Z0W1X0Y ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX0, ARX0, PermuteZWXY ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR1X, AR1X, PermuteWZYX ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(v) x b(v) = (r21, 0, -r01)
	// d(A) = h(A) dot abs(r21, 0, r01)
	// d(B) = h(B) dot abs(r12, 0, r10)
	d = XMVector3Dot( t, XMVectorPermute( RX1, -RX1, Permute0Z0W1X0Y ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX1, ARX1, PermuteZWXY ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR1X, AR1X, PermuteZWXY ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(v) x b(w) = (r22, 0, -r02)
	// d(A) = h(A) dot abs(r22, 0, r02)
	// d(B) = h(B) dot abs(r11, r10, 0)
	d = XMVector3Dot( t, XMVectorPermute( RX2, -RX2, Permute0Z0W1X0Y ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX2, ARX2, PermuteZWXY ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR1X, AR1X, PermuteYXWZ ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(w) x b(u) = (-r10, r00, 0)
	// d(A) = h(A) dot abs(r10, r00, 0)
	// d(B) = h(B) dot abs(0, r22, r21)
	d = XMVector3Dot( t, XMVectorPermute( RX0, -RX0, Permute1Y0X0W0Z ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX0, ARX0, PermuteYXWZ ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR2X, AR2X, PermuteWZYX ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(w) x b(v) = (-r11, r01, 0)
	// d(A) = h(A) dot abs(r11, r01, 0)
	// d(B) = h(B) dot abs(r22, 0, r20)
	d = XMVector3Dot( t, XMVectorPermute( RX1, -RX1, Permute1Y0X0W0Z ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX1, ARX1, PermuteYXWZ ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR2X, AR2X, PermuteZWXY ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// l = a(w) x b(w) = (-r12, r02, 0)
	// d(A) = h(A) dot abs(r12, r02, 0)
	// d(B) = h(B) dot abs(r21, r20, 0)
	d = XMVector3Dot( t, XMVectorPermute( RX2, -RX2, Permute1Y0X0W0Z ) );
	d_A = XMVector3Dot( h_A, XMVectorPermute( ARX2, ARX2, PermuteYXWZ ) );
	d_B = XMVector3Dot( h_B, XMVectorPermute( AR2X, AR2X, PermuteYXWZ ) );
	NoIntersection = XMVectorOrInt( NoIntersection,
									XMVectorGreater( XMVectorAbs( d ), XMVectorAdd( d_A, d_B ) ) );

	// No seperating axis found, boxes must intersect.
	return XMVector4NotEqualInt( NoIntersection, XMVectorTrueInt() ) != 0;
}

bool IntersectTriangleOrientedBox( const Vector3* triangleVertices, const Matrix& invOrientedBox )
{
	// put triangle points in "inverse bounding box" space
	Vector3 v[3];
	TransformCoords( v, triangleVertices, triangleVertices + 3, invOrientedBox );

	if( ( v[0].x < -1.f ) && ( v[1].x < -1.f ) && ( v[2].x < -1.f ) )
		return false;
	if( ( v[0].x > 1.f ) && ( v[1].x > 1.f ) && ( v[2].x > 1.f ) )
		return false;
	if( ( v[0].y < -1.f ) && ( v[1].y < -1.f ) && ( v[2].y < -1.f ) )
		return false;
	if( ( v[0].y > 1.f ) && ( v[1].y > 1.f ) && ( v[2].y > 1.f ) )
		return false;
	if( ( v[0].z < -1.f ) && ( v[1].z < -1.f ) && ( v[2].z < -1.f ) )
		return false;
	if( ( v[0].z > 1.f ) && ( v[1].z > 1.f ) && ( v[2].z > 1.f ) )
		return false;

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   A quick pre-test for triangle-AABB intersection. Can return false-positive answers.
// Arguments:
//   v0, v1, v2 - Triangle vertex positions
//   min, max - AABB
// Return Value:
//   false if triangle does not intersect AABB
//   true if triangle may intersect AABB
// --------------------------------------------------------------------------------------
bool IntersectTriangleAABB( const Vector3* v0,
							const Vector3* v1,
							const Vector3* v2,
							const Vector3& min,
							const Vector3& max )
{
	if( ( v0->x < min.x ) && ( v1->x < min.x ) && ( v2->x < min.x ) )
		return false;
	if( ( v0->x > max.x ) && ( v1->x > max.x ) && ( v2->x > max.x ) )
		return false;
	if( ( v0->y < min.y ) && ( v1->y < min.y ) && ( v2->y < min.y ) )
		return false;
	if( ( v0->y > max.y ) && ( v1->y > max.y ) && ( v2->y > max.y ) )
		return false;
	if( ( v0->z < min.z ) && ( v1->z < min.z ) && ( v2->z < min.z ) )
		return false;
	if( ( v0->z > max.z ) && ( v1->z > max.z ) && ( v2->z > max.z ) )
		return false;

	return true;
}

namespace
{

// --------------------------------------------------------------------------------------
// Description:
//   A helper function to test separating plane at a cross product between triangle edge
//   and one of sides of a unit AABB.
// Return Value:
//   false if separating plane was found
//   true otherwise
// --------------------------------------------------------------------------------------
inline bool AxisTest( float a, float b, float fa, float fb, const Vector3& v0, const Vector3& v1, int axis0, int axis1 )
{
	float p0 = a * v0[axis0] + b * v0[axis1];
	float p2 = a * v1[axis0] + b * v1[axis1];
	if( p2 < p0 )
	{
		std::swap( p0, p2 );
	}
	float rad = fa + fb;
	return !( p0 > rad || p2 < -rad );
}

}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if a triangle intersects an oriented bounding box.
// Arguments:
//   v0, v1, v2 - Triangle vertex positions
//   invOrientedBox - Inverse transform of a box (-1, -1, -1) - (1, 1, 1)
// Return Value:
//   false if triangle does not intersect the box
//   true if triangle intersects the box
// --------------------------------------------------------------------------------------
bool IntersectTriangleOrientedBox( const Vector3* v0,
								   const Vector3* v1,
								   const Vector3* v2,
								   const Matrix& invOrientedBox )
{
	// put triangle points in "inverse bounding box" space
	Vector3 v[3];
	v[0] = TransformCoord( *v0, invOrientedBox );
	v[1] = TransformCoord( *v1, invOrientedBox );
	v[2] = TransformCoord( *v2, invOrientedBox );

	// checks box sides as separating plane
	if( ( v[0].x < -1.f ) && ( v[1].x < -1.f ) && ( v[2].x < -1.f ) )
		return false;
	if( ( v[0].x > 1.f ) && ( v[1].x > 1.f ) && ( v[2].x > 1.f ) )
		return false;
	if( ( v[0].y < -1.f ) && ( v[1].y < -1.f ) && ( v[2].y < -1.f ) )
		return false;
	if( ( v[0].y > 1.f ) && ( v[1].y > 1.f ) && ( v[2].y > 1.f ) )
		return false;
	if( ( v[0].z < -1.f ) && ( v[1].z < -1.f ) && ( v[2].z < -1.f ) )
		return false;
	if( ( v[0].z > 1.f ) && ( v[1].z > 1.f ) && ( v[2].z > 1.f ) )
		return false;

	Vector3 e0 = v[1] - v[0];
	Vector3 e1 = v[2] - v[1];
	Vector3 e2 = v[0] - v[2];

	// check triangle plane as a separating plane
	Vector3 normal = Cross( e0, e1 );

	Vector3 vmin, vmax;
	for( int i = 0; i < 3; ++i )
	{
		if( normal[i] > 0 )
		{
			vmin[i] = -1 - v[0][i];
			vmax[i] = 1 - v[0][i];
		}
		else
		{
			vmin[i] = 1 - v[0][i];
			vmax[i] = -1 - v[0][i];
		}
	}
	if( Dot( vmin, normal ) > 0 )
	{
		return false;
	}
	if( Dot( vmax, normal ) < 0 )
	{
		return false;
	}

	// 9 more separating plane checks for cross products between triangle edges
	// and box sides
	float fex, fey, fez;

	fex = std::abs( e0.x );
	fey = std::abs( e0.y );
	fez = std::abs( e0.z );
	if( !AxisTest( e0.z, -e0.y, fez, fey, v[0], v[2], 1, 2 ) )
	{
		return false;
	}
	if( !AxisTest( -e0.z, e0.x, fez, fex, v[0], v[2], 0, 2 ) )
	{
		return false;
	}
	if( !AxisTest( e0.y, -e0.x, fey, fex, v[1], v[2], 0, 1 ) )
	{
		return false;
	}

	fex = std::abs( e1.x );
	fey = std::abs( e1.y );
	fez = std::abs( e1.z );
	if( !AxisTest( e1.z, -e1.y, fez, fey, v[0], v[2], 1, 2 ) )
	{
		return false;
	}
	if( !AxisTest( -e1.z, e1.x, fez, fex, v[0], v[2], 0, 2 ) )
	{
		return false;
	}
	if( !AxisTest( e1.y, -e1.x, fey, fex, v[0], v[1], 0, 1 ) )
	{
		return false;
	}

	fex = std::abs( e2.x );
	fey = std::abs( e2.y );
	fez = std::abs( e2.z );
	if( !AxisTest( e2.z, -e2.y, fez, fey, v[0], v[1], 1, 2 ) )
	{
		return false;
	}
	if( !AxisTest( -e2.z, e2.x, fez, fex, v[0], v[1], 0, 2 ) )
	{
		return false;
	}
	if( !AxisTest( e2.y, -e2.x, fey, fex, v[1], v[2], 0, 1 ) )
	{
		return false;
	}

	return true;
}

bool IsBoundingBoxEmpty( const Vector3& min, const Vector3& max )
{
	for( int i = 0; i < 3; ++i )
	{
		if( max[i] <= min[i] )
		{
			return true;
		}
	}

	return false;
}
Vector3 ClosestPointToBoundingBox( const Vector3& min, const Vector3& max, const Vector3& point )
{
	return Minimize( Maximize( point, min ), max );
}

CcpMath::AxisAlignedBox GetItemSetAabb( const CcpMath::AxisAlignedBox& staticBounds, const std::vector<std::pair<int, CcpMath::AxisAlignedBox>>& boneBounds, const Float4x3* bones, size_t boneCount )
{
	auto aabb = staticBounds;
	for( auto& box : boneBounds )
	{
		if( box.first < int( boneCount ) )
		{
			Matrix boneTF = IdentityMatrix();
			TriMatrixCopyFrom3x4( &boneTF, &bones[box.first] );
			auto boxAabb = box.second;
			boxAabb.Transform( boneTF );
			aabb.IncludeBox( boxAabb );
		}
		else
		{
			aabb.IncludeBox( box.second );
		}
	}
	return aabb;
}