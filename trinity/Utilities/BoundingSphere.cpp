#include "StdAfx.h"
#include "Include/TriMath.h"
#include "BoundingSphere.h"
#include "Utilities/Vector3d.h"
#include "Utilities/Vector4d.h"

void BoundingSphereInitialize( Vector4& sphere )
{
	sphere = Vector4( 0.f, 0.f, 0.f, 0.f );
}

bool BoundingSphereIsInside( const Vector4& sphere, const Vector3& pos )
{
	const float radiusEpsilon = 1e-4f;

	Vector3 delta = pos - ( const Vector3& )sphere;
	return ( LengthSq( delta ) <= sphere.w * sphere.w + radiusEpsilon );
}

bool BoundingSphereIsSphereInside( const Vector4& parentSphere, const Vector4& testSphere )
{
	// pre-chck radiuses
	if( parentSphere.w < testSphere.w )
	{
		return false;
	}
	Vector3 delta = ( const Vector3& )testSphere - ( const Vector3& )parentSphere;
	return ( LengthSq( delta ) <= ( parentSphere.w - testSphere.w ) * ( parentSphere.w - testSphere.w ) );
}

void BoundingSphereUpdate( const Vector3& pos, Vector4& sphere )
{
	// do not update if is inside
	if( BoundingSphereIsInside( sphere, pos ) )
	{
		return;
	}

	// extend sphere
	Vector3 delta = pos - ( Vector3& )sphere;
	float deltaLen = Length( delta );

	( Vector3& )sphere += 0.5f * ( 1.f - sphere.w / deltaLen ) * delta;
	sphere.w = 0.5f * ( sphere.w + deltaLen );
}

void BoundingSphereUpdate( const Vector4& addSphere, Vector4& resultSphere )
{
	// do not update if is inside
	if( BoundingSphereIsSphereInside( resultSphere, addSphere ) )
	{
		return;
	}
	if( BoundingSphereIsSphereInside( addSphere, resultSphere ) )
	{
		resultSphere = addSphere;
		return;
	}

	// extend sphere
	Vector3 delta = ( Vector3& )addSphere - ( Vector3& )resultSphere;
	float deltaLen = Length( delta );

	( Vector3& )resultSphere += 0.5f * ( 1.f + ( addSphere.w - resultSphere.w ) / deltaLen ) * delta;
	resultSphere.w = 0.5f * ( resultSphere.w + addSphere.w + deltaLen );
}

void BoundingSphereTransform( const Matrix& tf, Vector4& sphere )
{
	Vector3 center;
	// translate center
	sphere.GetXYZ() = TransformCoord( sphere.GetXYZ(), tf );
	// scale with highest scale factor
	float scaleX = Length( tf.GetX() );
	float scaleY = Length( tf.GetY() );
	float scaleZ = Length( tf.GetZ() );
	float scale = std::max( scaleX, std::max( scaleY, scaleZ ) );
	sphere.w *= scale;
}

void BoundingSphereTranslate( const Vector3& offset, Vector4& sphere )
{
	Vector3 center( sphere.x, sphere.y, sphere.z );
	sphere = Vector4( center + offset, sphere.w );
}

bool IntersectSphereAxisAlignedBox( const Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds )
{
	XMVECTOR SphereCenter = XMVectorSet( sphere.x, sphere.y, sphere.z, 0.0f );
	XMVECTOR SphereRadius = XMVectorReplicatePtr( &sphere.w );

	XMVECTOR BoxMin = XMVectorSet( minBounds.x, minBounds.y, minBounds.z, 0.0f );
	XMVECTOR BoxMax = XMVectorSet( maxBounds.x, maxBounds.y, maxBounds.z, 0.0f );

	// Find the distance to the nearest point on the box.
	// for each i in (x, y, z)
	// if (SphereCenter(i) < BoxMin(i)) d2 += (SphereCenter(i) - BoxMin(i)) ^ 2
	// else if (SphereCenter(i) > BoxMax(i)) d2 += (SphereCenter(i) - BoxMax(i)) ^ 2

	XMVECTOR d = XMVectorZero();

	// Compute d for each dimension.
	XMVECTOR LessThanMin = XMVectorLess( SphereCenter, BoxMin );
	XMVECTOR GreaterThanMax = XMVectorGreater( SphereCenter, BoxMax );

	XMVECTOR MinDelta = SphereCenter - BoxMin;
	XMVECTOR MaxDelta = SphereCenter - BoxMax;

	// Choose value for each dimension based on the comparison.
	d = XMVectorSelect( d, MinDelta, LessThanMin );
	d = XMVectorSelect( d, MaxDelta, GreaterThanMax );

	// Use a dot-product to square them and sum them together.
	XMVECTOR d2 = XMVector3Dot( d, d );

	return XMVector4LessOrEqual( d2, XMVectorMultiply( SphereRadius, SphereRadius ) ) != 0;
}

bool IntersectEllipsoidRay( Vector3& out, const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& rayOrigin, const Vector3& rayDir )
{
	float t;
	return IntersectEllipsoidRay( out, t, ellipsoidCenter, ellipsoidRadii, rayOrigin, rayDir );
}

bool IntersectEllipsoidRay( Vector3& out, float& t, const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& rayOrigin, const Vector3& rayDir )
{
	Vector3 v = Vector3( rayDir.x / ellipsoidRadii.x, rayDir.y / ellipsoidRadii.y, rayDir.z / ellipsoidRadii.z );
	Vector3 s = Vector3( ( rayOrigin.x - ellipsoidCenter.x ) / ellipsoidRadii.x, ( rayOrigin.y - ellipsoidCenter.y ) / ellipsoidRadii.y, ( rayOrigin.z - ellipsoidCenter.z ) / ellipsoidRadii.z );

	float v_v = Dot( v, v );
	float v_s = Dot( v, s );
	float s_s = Dot( s, s );
	float pq = ( v_s / v_v ) * ( v_s / v_v ) - ( s_s / v_v ) + 1.f / v_v;
	if( pq < 0.f )
	{
		return false;
	}
	pq = sqrt( pq );
	t = -pq - ( v_s / v_v );
	if( t < 0 )
	{
		t = pq - ( v_s / v_v );
	}
	out = t * rayDir + rayOrigin;
	return true;
}

bool IntersectEllipsoidRayClosest( Vector3& out, const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& rayOrigin, const Vector3& rayDir )
{
	Vector3 v = Vector3( rayDir.x / ellipsoidRadii.x, rayDir.y / ellipsoidRadii.y, rayDir.z / ellipsoidRadii.z );
	Vector3 s = Vector3( ( rayOrigin.x - ellipsoidCenter.x ) / ellipsoidRadii.x, ( rayOrigin.y - ellipsoidCenter.y ) / ellipsoidRadii.y, ( rayOrigin.z - ellipsoidCenter.z ) / ellipsoidRadii.z );

	float v_v = Dot( v, v );
	float v_s = Dot( v, s );
	float s_s = Dot( s, s );
	float pq = ( v_s / v_v ) * ( v_s / v_v ) - ( s_s / v_v ) + 1.f / v_v;
	if( pq < 0.f )
	{
		return false;
	}
	pq = sqrt( pq );
	float t1 = -pq - ( v_s / v_v );
	float t2 = pq - ( v_s / v_v );
	
	float minT = abs( t1 ) < abs( t2 ) ? t1 : t2;
	out = minT * rayDir + rayOrigin;
	return true;
}


bool IsPointInsideEllipsoid( const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& point )
{
	Vector3 d = point - ellipsoidCenter;
	d.x /= ellipsoidRadii.x;
	d.y /= ellipsoidRadii.y;
	d.z /= ellipsoidRadii.z;
	return LengthSq( d ) <= 1;
}

void BoundingSphereFromBox( Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds, const Matrix* tf )
{
	Vector3 min( minBounds );
	Vector3 max( maxBounds );

	if( tf )
	{
		min = TransformCoord( minBounds, *tf );
		max = TransformCoord( maxBounds, *tf );
	}

	sphere.GetXYZ() = min + max;
	min -= max;
	sphere.w = Length( min );
	sphere *= 0.5f;
}

// --------------------------------------------------------------------------------
// Description:
//   Local recursive function used by the bounding sphere generation algorithm
// --------------------------------------------------------------------------------
Vector4d recurBoundingSphereCreate( Vector3 const** p, size_t len, size_t b )
{
	const double radiusEpsilon = 1e-4;

	Vector4d mb;

	// in case of the "trivial" ones we have an easy solution
	switch( b )
	{
	case 0:
		BoundingSphereFromPoints( mb, nullptr, 0 );
		break;
	case 1:
		BoundingSphereFromPoints( mb, &p[-1], 1 );
		break;
	case 2:
		BoundingSphereFromPoints( mb, &p[-2], 2 );
		break;
	case 3:
		BoundingSphereFromPoints( mb, &p[-3], 3 );
		break;
	case 4:
		BoundingSphereFromPoints( mb, &p[-4], 4 );
		return mb;
	}

	for( size_t i = 0; i < len; ++i )
	{
		Vector3d delta = Vector3d( *p[i] ) - Vector3d( mb );
		// outside?
		if( LengthSq( delta ) > mb.w * mb.w + radiusEpsilon )
		{
			// avoid duplicates in the four points
			bool isDuplicate = false;
			for( size_t j = 0; j < b; ++j )
			{
				if( TriVectorIsIdentical( p[i], p[-(int32_t)j - 1] ) )
				{
					isDuplicate = true;
					break;
				}
			}
			if( !isDuplicate )
			{
				for( size_t j = i; j > 0; --j )
				{
					std::swap( p[j], p[j - 1] );
				}

				mb = recurBoundingSphereCreate( p + 1, i, b + 1 );
			}
		}
	}

	return mb;
}

// --------------------------------------------------------------------------------
// Description:
//   Create a bounding sphere by two given points
// --------------------------------------------------------------------------------
void BoundingSphereFromPoints( Vector4& sphere, const Vector3& point1, const Vector3& point2 )
{
	const float radiusEpsilon = 1e-4f;

	Vector3 a = 0.5f * ( point2 - point1 );
	sphere = Vector4( a + point1, Length( a ) + radiusEpsilon );
}

// --------------------------------------------------------------------------------
// Description:
//   Create a bounding sphere by a given number of points.
//   Based on http://hbf.github.io/miniball/seb.pdf or https://www.flipcode.com/archives/Smallest_Enclosing_Spheres.shtml
// --------------------------------------------------------------------------------
void BoundingSphereFromPoints( Vector4& sphere, Vector3 const** points, size_t pointsCount )
{
	Vector4d tempSphere = sphere;
	BoundingSphereFromPoints( tempSphere, points, pointsCount );
	sphere = tempSphere.AsVector4();
}

// --------------------------------------------------------------------------------
// Description:
//   Double precision version of BoundingSphereFromPoints()
// --------------------------------------------------------------------------------
void BoundingSphereFromPoints( Vector4d& sphere, Vector3 const** points, size_t pointsCount )
{
	const double radiusEpsilon = 1e-4;

	// if we have zero to four points, generation is easy!
	switch( pointsCount )
	{
	case 0:
		// init bounding sphere
		sphere = Vector4d( 0.0, 0.0, 0.0, 0.0 );
		break;
	case 1:
		{
			sphere = Vector4d( *points[0], (float)radiusEpsilon );
		}
		break;
	case 2:
		{
			// simple sphere srounf 2 points
			Vector3d a = 0.5 * ( Vector3d( *points[1] ) - Vector3d( *points[0] ) );
			sphere = Vector4d( a + Vector3d( *points[0] ), Length( a ) + radiusEpsilon );
		}
		break;
	case 3:
		{
			Vector3d a = Vector3d( *points[1] ) - Vector3d( *points[0] );
			Vector3d b = Vector3d( *points[2] ) - Vector3d( *points[0] );
			Vector3d axb, axbxa, bxaxb;
			axb = Cross( a, b );
			axbxa = Cross( axb, a );
			bxaxb = Cross( b, axb );
			double denom = 2.0 * Dot( axb, axb );
			if( denom == 0.0 )
			{
				// fail, must try with two points
				CCP_LOGWARN("BoundingSphereFromPoints: failed because denominator is zero! Using fallback...");
				return BoundingSphereFromPoints( sphere, points, 2 );
			}
			double a2 = Dot( a, a );
			double b2 = Dot( b, b );
			Vector3d o = ( b2 * axbxa + a2 * bxaxb ) / denom;
			sphere = Vector4d( o + Vector3d( *points[0] ), Length( o ) + radiusEpsilon );
		}
		break;
	case 4:
		{
			Vector3d a = Vector3d( *points[1] ) - Vector3d( *points[0] );
			Vector3d b = Vector3d( *points[2] ) - Vector3d( *points[0] );
			Vector3d c = Vector3d( *points[3] ) - Vector3d( *points[0] );
			double denom = 2.0 * ( a.x * (b.y * c.z - c.y * b.z) - b.x * (a.y * c.z - c.y * a.z) + c.x * (a.y * b.z - b.y * a.z) );
			if( denom == 0.0 )
			{
				// fail, must try with three points
				CCP_LOGWARN("BoundingSphereFromPoints: failed because denominator is zero! Using fallback...");
				return BoundingSphereFromPoints( sphere, points, 3 );
			}
			double a2 = Dot( a, a );
			double b2 = Dot( b, b );
			double c2 = Dot( c, c );
			Vector3d axb = Cross( a, b );
			Vector3d cxa = Cross( c, a );
			Vector3d bxc = Cross( b, c );
			Vector3d o = ( c2 * axb + b2 * cxa + a2 * bxc ) / denom;
			sphere = Vector4d( o + Vector3d( *points[0] ), Length( o ) + radiusEpsilon );
	}
		break;
	default:
		sphere = recurBoundingSphereCreate( points, pointsCount, 0 );
		break;
	}
}



