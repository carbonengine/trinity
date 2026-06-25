// Copyright © 2023 CCP ehf.

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
			d += a * a;
		}
		else if( pCenter[i] > pMax[i] )
		{
			float a = pCenter[i] - pMax[i];
			d += a * a;
		}
	}

	float r2 = radius * radius;
	if( d > r2 )
	{
		return false;
	}

	return true;
}

bool TriFrustumOrtho::IsSphereVisibleIgnoreFarPlane( const Vector3& center, float radius ) const
{
	Vector3 centerInView = TransformCoord( center, m_view );

	float d = 0;

	float* pCenter = (float*)&centerInView;
	float* pMax = (float*)&m_boundsMax;
	float* pMin = (float*)&m_boundsMin;
	for( int i = 0; i < 3; ++i )
	{
		if( pCenter[i] < pMin[i] )
		{
			float a = pCenter[i] - pMin[i];
			d += a * a;
		}
		else if( i != 2 && pCenter[i] > pMax[i] )
		{
			float a = pCenter[i] - pMax[i];
			d += a * a;
		}
	}

	float r2 = radius * radius;
	if( d > r2 )
	{
		return false;
	}

	return true;
}

TriFrustumTestResult TriFrustumOrtho::SphereTestIgnoreFarPlane( const CcpMath::Sphere& sphere ) const
{
	TriFrustumTestResult result;
	auto centerInView = XMVectorSetW( XMVector3TransformCoord( sphere.center, m_view ), 0 );
	XMVECTOR boundsMin = m_boundsMin;
	XMVECTOR boundsMax = m_boundsMax;

	auto fromMin = XMVectorSubtract( centerInView, boundsMin );
	auto outsideMin = XMVectorMin( fromMin, XMVectorZero() );
	auto sum = XMVectorMultiply( outsideMin, outsideMin );

	auto fromMax = XMVectorSubtract( boundsMax, centerInView );
	auto outsideMax = XMVectorMin( fromMax, XMVectorZero() );

	// because we are ignoring far plane, zero out Z component
	outsideMax = XMVectorSetZ( outsideMax, 0 );

	sum = XMVectorMultiplyAdd( outsideMax, outsideMax, sum );

	float d = XMVectorGetX( XMVectorSum( sum ) );
	if( d > sphere.radius * sphere.radius )
	{
		result = TriFrustumTestResult::Outside;
	}
	else
	{
		auto radius = XMVectorSet( sphere.radius, sphere.radius, sphere.radius, 0 );
		auto insideMin = XMVectorGreaterOrEqual( fromMin, radius );
		auto insideMax = XMVectorGreaterOrEqual( XMVectorSetZ( fromMax, sphere.radius ), radius );
		auto isInside = XMVectorAndInt( insideMin, insideMax );
		if( XMVector3EqualInt( isInside, XMVectorTrueInt() ) )
		{
			result = TriFrustumTestResult::Inside;
		}
		else
		{
			result = TriFrustumTestResult::Intersect;
		}
	}
	return result;
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
