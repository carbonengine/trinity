// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriFrustum.h"
#include "TriViewport.h"
#include "TriSettingsRegistrar.h"
#include "Tr2Renderer.h"

bool g_frustumCullingDisabled = false;
TRI_REGISTER_SETTING( "frustumCullingDisabled", g_frustumCullingDisabled );

TriFrustum::TriFrustum() :
	m_halfWidthProjection( 0 ),
	m_zNear( 0 ),
	m_zFar( 0 ),
	m_aspectRatio( 1.0f ),
	m_fov( 1.0f )
{
#ifdef TRINITYDEV
	m_frustumTestCounter = 0;
	m_frustumRejectionCounter = 0;
	m_frustumCullingRatio = 0.0f;
#endif
}

void TriFrustum::DeriveFrustum( const Matrix* view, const Vector3* campos, const Matrix* projection, const TriViewport& viewport )
{
	CacheTransformationData( view, campos, projection, viewport );

	// It does not make any sense of doing frustum culling in any other space then world space.
	// Otherwise we would have to move all the geometry to camera space before we would do any culling.
	Matrix worldMat = *view * *projection;

	// Now do the actual extraction
	ExtractFrustum( &worldMat );
}

void TriFrustum::ExtractFrustum( const Matrix* proj )
{ /**
	proj	-	The projection matrix to extract a frustum from.
 */
#ifdef TRINITYDEV
	if( m_frustumTestCounter != 0 )
		m_frustumCullingRatio = (float)m_frustumRejectionCounter / (float)m_frustumTestCounter;
	m_frustumRejectionCounter = 0;
	m_frustumTestCounter = 0;
#endif
	// Frustum Extraction
	// This is actually very simple when you understand all the components of the projection matrix.
	// dRatio = far/(far - near )
	// Columns
	// X->( (1/tan(fov/2))/aspectratio, 0, 0 ). You need to scale all the input values by the 1/tan, because when the 'fov' changes you want to see more.
	//											We also want to make sure that the values are not stretched when the screen is not a perfect square. That is why
	//											we like to scale the x values by the 1/aspectRatio.
	// Y->( 0 1/tan(fov/2), 0 )					The Y values need scaling to, but they do not have to be changed by the aspect ratio because the aspect ratio is a scale relating the
	//											size of 'Y' to x values. x/Y.
	//
	// Z->( 0, 0, dRatio, -near*dRatio )		The depth values get mapped to the z-buffer between the ranges of 0.0 - 1.0. To ensure that no values get divided by zero
	//											we use the near plane and store the actual z value in the w member of the output vector. No value get projected to 2D space before
	//											the z-buffer test, so the projection matrix does not project anything. It basically just scales the values so they can be projected
	//											in a simple homogeneous fashion, x/w, y/w, z/w by a frustum with a 90 degree fov.
	//											To map the depth values correctly to the z-buffer and make sure we don't divide by zero we subtract the near plane from the z-value, so
	//											if the z-value was behind the near plane the sign of the value would switch. Then we need to scale it back to where it was to get the
	//											correct z-buffer value. We want all our values to be mapped correctly between 0.0 - 1.0 where 0.0 is our near plane and 1.0 is our far plane.
	//											So we can't just subtract the nearplane then divide that by the distance between the near and far plane, because than the depth values that
	//											would lie on or behind the farplane(by distance equal to the nearplane) would get drawn. So the value we need to scale the value back into place
	//											after we have subtracted the nearplane is the ratio between the distance to the farplane and the distance between the near and far plane.
	//											*)dRatio = farplane/ (farplane - nearplane )
	//											*)zbuffer = z - nearplane*dRatio
	//											We can split this formula up in the projection matrix to correctly store the z-buffer information in the z member of the output vector.
	//											z-nearplane*dRatio = z*dRation - nearplane*dRatio. As you can see in the third column of the projection matrix, and given that the input vector
	//											is homogeneous with a w=1.0.
	//
	// W->( 0, 0, 1.0, 0 )						All this does is copy the depth value to the w member of the output vector.
	//
	// So to extract the frustums we only need to add and subtract column vectors to get the normals.

	// front
	// The normal of the near plane is the same as the w-component in the projection matrix
	m_planes[PLANE_FRONT].a = ( proj->_13 );
	m_planes[PLANE_FRONT].b = ( proj->_23 );
	m_planes[PLANE_FRONT].c = ( proj->_33 );
	m_planes[PLANE_FRONT].d = ( proj->_43 ); // This will produce the correct number when it is divided by the length of the normal
	//left
	m_planes[PLANE_LEFT].a = ( proj->_14 + proj->_11 );
	m_planes[PLANE_LEFT].b = ( proj->_24 + proj->_21 );
	m_planes[PLANE_LEFT].c = ( proj->_34 + proj->_31 );
	m_planes[PLANE_LEFT].d = ( proj->_44 + proj->_41 );
	//top
	m_planes[PLANE_TOP].a = ( proj->_14 - proj->_12 );
	m_planes[PLANE_TOP].b = ( proj->_24 - proj->_22 );
	m_planes[PLANE_TOP].c = ( proj->_34 - proj->_32 );
	m_planes[PLANE_TOP].d = ( proj->_44 - proj->_42 );
	//right
	m_planes[PLANE_RIGHT].a = ( proj->_14 - proj->_11 );
	m_planes[PLANE_RIGHT].b = ( proj->_24 - proj->_21 );
	m_planes[PLANE_RIGHT].c = ( proj->_34 - proj->_31 );
	m_planes[PLANE_RIGHT].d = ( proj->_44 - proj->_41 );
	//bottom
	m_planes[PLANE_BOTTOM].a = ( proj->_14 + proj->_12 );
	m_planes[PLANE_BOTTOM].b = ( proj->_24 + proj->_22 );
	m_planes[PLANE_BOTTOM].c = ( proj->_34 + proj->_32 );
	m_planes[PLANE_BOTTOM].d = ( proj->_44 + proj->_42 );
	//back
	m_planes[PLANE_BACK].a = ( proj->_14 - proj->_13 );
	m_planes[PLANE_BACK].b = ( proj->_24 - proj->_23 );
	m_planes[PLANE_BACK].c = ( proj->_34 - proj->_33 );
	// The distance for the far plane is extracted from the normal when the plane is normalized
	// 1)clipratio = fardistance/(fardistance-neardistance)
	// neardistance = -(m44/clipratio)
	// The m33 component is the value that will scale the 'z' value back to correct place between 0 - farplane, after the distance to the nearplane has been subtracted from it.
	// The m33 tells us how much larger the distance of the farplane to the origin is compared to the distance from the farplane to the frontplane.
	m_planes[PLANE_BACK].d = ( proj->_44 - proj->_43 );

	for( int i = 0; i < PLANE_COUNT; i++ )
	{
		m_planes[i] = Normalize( m_planes[i] );
	}
}

bool TriFrustum::IsSphereVisible( const Vector4* sphere, bool cullBackPlane ) const
{
	return IsSphereVisible( *reinterpret_cast<const Vector3*>( sphere ), sphere->w, cullBackPlane );
}

// ---------------------------------------------------------------------------
bool TriFrustum::IsSphereVisible( const Vector3& center, float radius, bool cullBackPlane ) const
{
	if( g_frustumCullingDisabled )
	{
		return true;
	}
#ifdef TRINITYDEV
	m_frustumTestCounter++;
#endif
	// For some reason the old code ignored the back plane. I don't know why!!
	for( int i = 0; i < ( PLANE_COUNT - 1 ) + cullBackPlane; i++ )
	{
		if( DotCoord( m_planes[i], center ) < -radius )
		{
#ifdef TRINITYDEV
			m_frustumRejectionCounter++;
#endif
			return false;
		}
	}
	return true;
}

TriFrustumTestResult TriFrustum::SphereTest( const CcpMath::Sphere& sphere ) const
{
	TriFrustumTestResult result = TriFrustumTestResult::Inside;
	for( int i = 0; i < PLANE_COUNT; i++ )
	{
		auto d = DotCoord( m_planes[i], sphere.center );
		if( d < -sphere.radius )
		{
			return TriFrustumTestResult::Outside;
		}
		if( d < sphere.radius )
		{
			result = TriFrustumTestResult::Intersect;
		}
	}
	return result;
}

bool TriFrustum::IsPointVisible( const Vector3* point ) const
{ /**
	Wrapping the sphere visible with a radius of zero.
 */
	Vector4 boundingSphere( *point, 0.f );
	return IsSphereVisible( &boundingSphere );
}

// -------------------------------------------------------------
// Description:
//   Tests for frustum-AABB intersection. May return false
//   positive answers.
// Arguments:
//   boundsMin - Min bounds of AABB
//   boundsMax - Max bounds of AABB
// Return Value:
//   true If AABB intersects / is inside frustum
//   false If AABB is outside frustum
// -------------------------------------------------------------
bool TriFrustum::IsBoxVisible( const Vector3& boundsMin, const Vector3& boundsMax ) const
{
	Vector3 vmax;

	for( int i = 0; i < 6; ++i )
	{
		if( m_planes[i].a > 0 )
		{
			vmax.x = boundsMax.x;
		}
		else
		{
			vmax.x = boundsMin.x;
		}
		if( m_planes[i].b > 0 )
		{
			vmax.y = boundsMax.y;
		}
		else
		{
			vmax.y = boundsMin.y;
		}
		if( m_planes[i].c > 0 )
		{
			vmax.z = boundsMax.z;
		}
		else
		{
			vmax.z = boundsMin.z;
		}
		if( DotCoord( m_planes[i], vmax ) < 0 )
		{
			return false;
		}
	}
	return true;
}

bool TriFrustum::IsBoxVisible( const CcpMath::AxisAlignedBox& aabb ) const
{
	if( !aabb )
	{
		return false;
	}
	return IsBoxVisible( aabb.m_min, aabb.m_max );
}

float TriFrustum::GetPixelSizeAccross( const Vector4* sphere ) const
{
	return GetPixelSizeAccross( *reinterpret_cast<const Vector3*>( sphere ), sphere->w );
}

float TriFrustum::GetPixelSizeAccrossEst( const Vector4* sphere ) const
{
	return GetPixelSizeAccrossEst( *reinterpret_cast<const Vector3*>( sphere ), sphere->w );
}

// ---------------------------------------------------------------------------
float TriFrustum::GetPixelSizeAccross( const Vector3& center, float radius ) const
{
	Vector3 d( center - m_viewPos );

	d = -d;

	if( LengthSq( d ) < radius * radius )
	{
		return std::numeric_limits<float>::max();
	}

	float depth = m_viewDir.x * d.x + m_viewDir.y * d.y + m_viewDir.z * d.z;
	// clamp values close to zero and below
	const float epsilon = 1e-5f;
	if( depth < epsilon )
	{
		depth = epsilon;
	}

	if( radius < epsilon )
	{
		return 0.0f;
	}

	float ratio = radius / depth;

	return ( ratio * m_halfWidthProjection ) * 2.f;
}

float TriFrustum::GetPixelSizeAccrossEst( const Vector3& center, float radius ) const
{
	if( radius <= 0 )
	{
		return 0;
	}
	Vector3 d( center - m_viewPos );

	float lengthSqrd = LengthSq( d );
	float radiusSqrd = radius * radius;
	if( lengthSqrd < radius * radius )
	{
		//The camera is inside the object, it essentially has infinite screen size.
		return std::numeric_limits<float>::max();
	}

	//adjusted distance based on the visible part of the sphere, that properly goes to infinity as you enter the sphere.
	float distance = sqrt( lengthSqrd - radiusSqrd );
	return ( radius / distance * m_halfWidthProjection * 2.0f );
}

float TriFrustum::GetPixelSizeAccross( const CcpMath::Sphere& sphere ) const
{
	return GetPixelSizeAccrossEst( sphere.center, sphere.radius );
}

float TriFrustum::GetPixelSizeAccross( const CcpMath::AxisAlignedBox& box ) const
{
	if( !box )
	{
		return 0;
	}
	return GetPixelSizeAccross( CcpMath::Sphere( box ) );
}

void TriFrustum::CacheTransformationData( const Matrix* view, const Vector3* campos, const Matrix* projection, const TriViewport& viewport )
{
	/**
	Store the view and projection information that is needed for extracting the pixel diameter.
	*/
	// Cache viewport data and projection information for extracting pixel coverage
	m_projectionMatrix = *projection;
	m_viewPos = *campos;

	m_viewDir.x = view->_13;
	m_viewDir.y = view->_23;
	m_viewDir.z = view->_33;

	m_halfWidthProjection = projection->_11 * viewport.width * 0.5f;

	DeconstructProjectionMatrix( *projection, m_aspectRatio, m_fov, m_zNear, m_zFar );
}
