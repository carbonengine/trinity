#include "StdAfx.h"
#include "TriFrustumOrtho.h"

TriFrustumOrtho::TriFrustumOrtho() :
	m_boundsMin( 0.0f, 0.0f, 0.0f ),
	m_boundsMax( 0.0f, 0.0f, 0.0f )
{
	D3DXMatrixIdentity( &m_view );
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
	Vector3 centerInView;
	D3DXVec3TransformCoord( &centerInView, &center, &m_view );

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