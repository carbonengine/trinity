// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "EveSphereVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "include/TriMath.h"

EveSphereVolume::EveSphereVolume( IRoot* lockobj ) :
	m_innerSphere( Vector3( 0.0f, 0.0f, 0.0f ), 1.0f ),
	m_outerSphere( Vector3( 0.0f, 0.0f, 0.0f ), 1.0f ),
	m_nextCallbackID( 1 )
{
}

EveSphereVolume::~EveSphereVolume()
{
}

void EveSphereVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const Color& baseColor )
{
	renderer.DrawSphere( this, TranslationMatrix( m_outerSphere.center ) * parentTransform, m_outerSphere.radius, 20, Tr2DebugRenderer::Wireframe, baseColor * 0.5f );
	renderer.DrawSphere( this, TranslationMatrix( m_outerSphere.center + m_innerSphere.center ) * parentTransform, m_innerSphere.radius, 20, Tr2DebugRenderer::Wireframe, baseColor * 0.6f );
}

const CcpMath::Sphere EveSphereVolume::GetBoundingSphere() const
{
	return m_outerSphere;
}

float EveSphereVolume::GetIntensity( Vector3 position )
{
	if( m_outerSphere.IsPointInside( position ) )
	{
		// since the innersphere is offset from the outer sphere, we need to construct a new one that is centered in the owners object space
		CcpMath::Sphere innerModified( m_innerSphere );
		innerModified.center += m_outerSphere.center;
		if( innerModified.IsPointInside( position ) )
		{
			return 1.0f;
		}
		// this will not handle inner sphere offsets...
		// will need a more complex solution for that
		float distFromInnnerCenter = LengthSq( position - m_outerSphere.center );
		float distFromInnerSurface = distFromInnnerCenter - pow( innerModified.radius, 2.0f );

		float interpolationDistance = pow( m_outerSphere.radius, 2.0f ) - pow( innerModified.radius, 2.0f );
		return 1.0f - distFromInnerSurface / interpolationDistance;
	}
	return 0.0f;
}

void EveSphereVolume::GeneratePointsInVolume( std::vector<Vector3>& points, size_t howManyToAdd, bool excludeInnerVolume, float fallOffFactor )
{
	Vector3 position;
	Quaternion rotation;
	float circleDiffRange = m_outerSphere.radius - m_innerSphere.radius;
	points.reserve( points.size() + howManyToAdd );

	if( excludeInnerVolume )
	{
		for( size_t i = 0; i < howManyToAdd; i++ )
		{
			float dist = m_innerSphere.radius + circleDiffRange * pow( (float)rand() / RAND_MAX, 1.f / 3.f );

			float a = TRI_2PI * ( float( rand() ) / RAND_MAX );
			float z = ( float( rand() ) / RAND_MAX ) * 2.f - 1.f;
			Vector3 angle( sqrt( 1.f - z * z ) * cos( a ), sqrt( 1.f - z * z ) * sin( a ), z );

			position = angle * dist;

			points.push_back( position );
		}
	}
	else
	{
		// as the outer volume is only half filled we use the difference multiplied by 0.5 (and pi is a shared constant so we skip it)
		float sizeDifference = pow( m_innerSphere.radius, 2.f ) / pow( m_innerSphere.radius + 0.5f * circleDiffRange, 2.f );
		float dist = 0.f;

		for( size_t i = 0; i < howManyToAdd; i++ )
		{
			if( (float)rand() / RAND_MAX < sizeDifference )
			{
				// inner volume
				dist = m_innerSphere.radius * pow( (float)rand() / RAND_MAX, 1.f / 3.f );
			}
			else
			{
				// outer volume
				dist = m_innerSphere.radius + circleDiffRange * pow( (float)rand() / RAND_MAX, fallOffFactor );
			}

			float a = TRI_2PI * ( float( rand() ) / RAND_MAX );
			float z = ( float( rand() ) / RAND_MAX ) * 2.f - 1.f;
			Vector3 angle( sqrt( 1.f - z * z ) * cos( a ), sqrt( 1.f - z * z ) * sin( a ), z );

			position = angle * dist;

			points.push_back( position );
		}
	}
}

uint32_t EveSphereVolume::RegisterForChanges( const std::function<void()>& callBack )
{
	m_onChangeCallbacks[m_nextCallbackID] = callBack;
	return m_nextCallbackID++;
}

void EveSphereVolume::UnregisterForChanges( uint32_t callbackID )
{
	m_onChangeCallbacks.erase( callbackID );
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveSphereVolume::OnModified( Be::Var* val )
{

	if( IsMatch( val, m_innerSphere.radius ) )
	{
		if( m_innerSphere.radius > m_outerSphere.radius )
		{
			m_outerSphere.radius = m_innerSphere.radius;
		}
	}

	if( IsMatch( val, m_outerSphere.radius ) )
	{
		m_outerSphere.radius = max( 0.f, m_outerSphere.radius );

		if( m_innerSphere.radius > m_outerSphere.radius )
		{
			m_innerSphere.radius = m_outerSphere.radius;
		}
	}

	for( auto callBack : m_onChangeCallbacks )
	{
		if( callBack.second != nullptr )
		{
			callBack.second();
		}
	}

	return true;
}
