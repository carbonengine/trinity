#include "StdAfx.h"
#include "TriFrustumOrtho.h"

TriFrustumOrtho::TriFrustumOrtho() :
	m_boundsMin( 0.0f, 0.0f, 0.0f ),
	m_boundsMax( 0.0f, 0.0f, 0.0f ),
	m_view( IdentityMatrix() )
{
}

void TriFrustumOrtho::DeriveFrustum( const Matrix& view, const Vector3& minBounds, const Vector3& maxBounds )
{
	m_view = view;
	m_boundsMin = minBounds;
	m_boundsMax = maxBounds;
}

bool TriFrustumOrtho::IsSphereVisibleAndInsideNearPlane( const Vector4* sphere ) const
{
	return IsSphereVisibleAndInsideNearPlane( *reinterpret_cast<const Vector3*>( sphere ), sphere->w );
}

// -----------------------------------------
bool TriFrustumOrtho::IsSphereVisibleAndInsideNearPlane( const Vector3& center, float radius ) const
{
	Vector3 centerInView = TransformCoord( center, m_view );

	if( centerInView.z - radius > m_boundsMax.z )
	{
		return false;
	}

	float d = 0;

	float* pCenter = (float*)&centerInView;
	float* pMax = (float*)&m_boundsMax;
	float* pMin = (float*)&m_boundsMin;
	for( int i = 0; i < 3; ++i )
	{
		if( pCenter[i] < pMin[i] )
		{
			float a = pCenter[i] - pMin[i];
			d += a*a;
		}
		else if( pCenter[i] > pMax[i] )
		{
			float a = pCenter[i] - pMax[i];
			d += a*a;
		}
	}
	
	float r2 = radius * radius;
	if( d > r2 )
	{
		return false;
	}

	return true;
}

float TriFrustumOrtho::GetPixelSize( Vector4 sphere, uint16_t textureSize ) const
{
	Vector4 d = sphere;
	float frustumWidth = m_boundsMax.x - m_boundsMin.x;
	float frustumHeight = m_boundsMax.y - m_boundsMin.y;

	float dx = ( d.w * 2 ) / frustumWidth;
	float dy = ( d.w * 2 ) / frustumHeight;

	float larger = std::max( dx, dy );

	float shadowPixelSize = ( larger * textureSize );
	return shadowPixelSize;
}

const Vector3& TriFrustumOrtho::GetEyePos() const
{
	return m_view.GetTranslation();
}
