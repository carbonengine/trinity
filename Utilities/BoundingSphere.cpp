#include "StdAfx.h"
#include "BoundingSphere.h"

void BoundingSphereInitialize( Vector4& sphere )
{
	sphere = Vector4( 0.f, 0.f, 0.f, 0.f );
}

bool BoundingSphereIsInside( const Vector4& sphere, const Vector3& pos )
{
	Vector3 delta = pos - ( const Vector3& )sphere;
	return ( D3DXVec3LengthSq( &delta ) <= sphere.w * sphere.w );
}

bool BoundingSphereIsSphereInside( const Vector4& parentSphere, const Vector4& testSphere )
{
	// pre-chck radiuses
	if( parentSphere.w < testSphere.w )
	{
		return false;
	}
	Vector3 delta = ( const Vector3& )testSphere - ( const Vector3& )parentSphere;
	return ( D3DXVec3LengthSq( &delta ) <= ( parentSphere.w - testSphere.w ) * ( parentSphere.w - testSphere.w ) );
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
	float deltaLen = D3DXVec3Length( &delta );

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
	float deltaLen = D3DXVec3Length( &delta );

	( Vector3& )resultSphere += 0.5f * ( 1.f + ( addSphere.w - resultSphere.w ) / deltaLen ) * delta;
	resultSphere.w = 0.5f * ( resultSphere.w + addSphere.w + deltaLen );
}

void BoundingSphereTransform( const Matrix& tf, Vector4& sphere )
{
	Vector3 center;
	// translate center
	D3DXVec3TransformCoord( (Vector3*)&sphere, (Vector3*)&sphere, &tf );
	// scale with highest scale factor
	float scaleX = D3DXVec3Length( &tf.GetX() );
	float scaleY = D3DXVec3Length( &tf.GetY() );
	float scaleZ = D3DXVec3Length( &tf.GetZ() );
	float scale = std::max( scaleX, std::max( scaleY, scaleZ ) );
	sphere.w *= scale;
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

void BoundingSphereFromBox( Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds, const Matrix* tf )
{
	Vector3 min( minBounds );
	Vector3 max( maxBounds );

	if( tf )
	{
		D3DXVec3TransformCoord( &min, &minBounds, tf );
		D3DXVec3TransformCoord( &max, &maxBounds, tf );
	}

	D3DXVec3Add( (Vector3*)&sphere, &min, &max );
	D3DXVec3Subtract( &min, &min, &max );
	sphere.w = D3DXVec3Length( &min );
	sphere *= 0.5f;
}
