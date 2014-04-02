#include "StdAfx.h"
#include "ViewDistanceInfo.h"
#include "../TriFrustum.h"
#include "../TriSettingsRegistrar.h"

float g_nearClipMin = 6.f;
float g_nearClipMax = 99999.f;

float g_farClipMin = 100000.f;
float g_farClipMax = 10000000.f;

TRI_REGISTER_SETTING( "minNearClip", g_nearClipMin );
TRI_REGISTER_SETTING( "maxNearClip", g_nearClipMax );
TRI_REGISTER_SETTING( "minFarClip", g_farClipMin );
TRI_REGISTER_SETTING( "maxFarClip", g_farClipMax );

ViewDistanceInfo::ViewDistanceInfo() :
	m_far( g_farClipMin ),
	m_near( g_nearClipMax ),
	m_nearClipMin( g_nearClipMin ),
	m_farClipMax( g_farClipMax )
{
}
	
ViewDistanceInfo::ViewDistanceInfo( float nearClipMin, float farClipMax ) :
	m_nearClipMin( nearClipMin ),
	m_farClipMax( farClipMax )
{
	m_near = ( nearClipMin + farClipMax ) * 0.5f;
	m_far = m_near + 1.f;
}

static inline void GetNearAndFarFromBounds( const Vector4& boundingSphere, const TriFrustum& frustum, float& nearOut, float& farOut, float& distOut )
{
	Vector3 diff;
	D3DXVec3Subtract(&diff, &frustum.m_viewPos, (Vector3*)&boundingSphere);

	distOut = D3DXVec3Dot( &diff, &frustum.m_viewDir );
	farOut = distOut + boundingSphere.w;
	nearOut = distOut - boundingSphere.w;
}

void ViewDistanceInfo::UpdateClipPlanes( Vector4 boundingSphere, const TriFrustum& frustum )
{
	if( frustum.IsSphereVisible( &boundingSphere ) )
	{
		float f;
		float n;
		float center;
		GetNearAndFarFromBounds( boundingSphere, frustum, n, f, center );
		if( n <= m_farClipMax && f >= m_nearClipMin )
		{
			m_far = min( m_farClipMax, max( m_far, f ) );
			m_near = max( m_nearClipMin, min( m_near, n ) );
		}
	}
}

void ViewDistanceInfo::UpdateClipPlanesIfInFront( Vector4 boundingSphere, const TriFrustum& frustum )
{
	if( frustum.IsSphereVisible( &boundingSphere ) )
	{
		float f;
		float n;
		float center;
		GetNearAndFarFromBounds( boundingSphere, frustum, n, f, center );
		if( n <= m_farClipMax && f >= m_nearClipMin && center > 0 )
		{
			m_far = min( m_farClipMax, max( m_far, f ) );
			m_near = max( m_nearClipMin, min( m_near, n ) );
		}
	}
}
