#pragma once
#ifndef BoundingSphere_H
#define BoundingSphere_H

// helper functions to deal with bounding spheres
inline const Vector3& BoundingSphereGetCenter( const Vector4& sphere ) { return ( const Vector3& )sphere; }
void BoundingSphereInitialize( Vector4& sphere );
bool BoundingSphereIsInside( const Vector4& sphere, const Vector3& pos );
bool BoundingSphereIsSphereInside( const Vector4& parentSphere, const Vector4& testSphere );
void BoundingSphereUpdate( const Vector3& pos, Vector4& sphere );
void BoundingSphereUpdate( const Vector4& addSphere, Vector4& resultSphere );
void BoundingSphereTransform( const Matrix& tf, Vector4& sphere );
void BoundingSphereTranslate( const Vector3& offset, Vector4& sphere );
void BoundingSphereFromBox( Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds, const Matrix* tf = NULL );
void BoundingSphereFromPoints( Vector4& sphere, const Vector3& point1, const Vector3& point2 );
void BoundingSphereFromPoints( Vector4& sphere, Vector3 const** points, size_t pointsCount );
void BoundingSphereFromPoints( Vector4d& sphere, Vector3 const** points, size_t pointsCount );

bool IntersectSphereAxisAlignedBox( const Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds );
bool IntersectEllipsoidRayClosest( Vector3& out, const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& rayOrigin, const Vector3& rayDir );
bool IntersectEllipsoidRay( Vector3& out, const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& rayOrigin, const Vector3& rayDir );
bool IntersectEllipsoidRay( Vector3& out, float& t, const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& rayOrigin, const Vector3& rayDir );
bool IsPointInsideEllipsoid( const Vector3& ellipsoidCenter, const Vector3& ellipsoidRadii, const Vector3& point );

inline void BoundingSphereSetOrUpdate( Vector4& addSphere, Vector4& resultSphere, bool update )
{
	if( update )
	{
		BoundingSphereUpdate( addSphere, resultSphere );
	}
	else
	{
		resultSphere = addSphere;
	}
}


#endif // BoundingSphere_H