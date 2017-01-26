#pragma once
#ifndef TRIFRUSTUM_H
#define TRIFRUSTUM_H

BLUE_DECLARE( TriViewport );

class TriFrustum
{
public:

	enum
	{
		PLANE_FRONT = 0,
		PLANE_LEFT,
		PLANE_TOP,
		PLANE_RIGHT,
		PLANE_BOTTOM,
		PLANE_BACK,
		PLANE_COUNT
	};
	
	D3DXPLANE m_planes[ PLANE_COUNT ];
	Matrix m_projectionMatrix;

	Vector3	m_viewPos;
	Vector3 m_viewDir;

	float m_halfWidthProjection;

	float m_zNear, m_zFar;
	float m_aspectRatio, m_fov;

#ifdef TRINITYDEV
	mutable int	m_frustumTestCounter;
	mutable int	m_frustumRejectionCounter;
	float m_frustumCullingRatio;
#endif

	TriFrustum();
	// Extract frustum planes from the view and projection matrix in world coordinates
	void DeriveFrustum( const Matrix* view, const Vector3* campos, const Matrix* projection, const TriViewport& viewport );
	
	// Returns true if any part of the sphere is inside the frustum
	bool IsSphereVisible( const Vector4* sphere, bool cullBackPlane = false ) const;
	bool IsSphereVisible( const Vector3& center, float radius, bool cullBackPlane = false ) const;

	// Returns true if the point is inside the frustum
	bool IsPointVisible( const Vector3* point ) const;

	// Returns false if AABB is outside the frustum
	bool IsBoxVisible( const Vector3& boundsMin, const Vector3& boundsMax ) const;

	// Get the pixel coverage of a bounding sphere on screen
	float GetPixelSizeAccross( const Vector4* sphere ) const;
	float GetPixelSizeAccross( const Vector3& center, float radius ) const;

	void ExtractFrustum( const Matrix* proj );

private:
	void CacheTransformationData( const Matrix* view, const Vector3* campos, const Matrix* projection, const TriViewport& viewport );
};

#endif // TRIFRUSTUM_H