////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "EveSphereVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"

EveSphereVolume::EveSphereVolume( IRoot* lockobj ) :
	m_innerSphere( Vector3(0.0f, 0.0f, 0.0f), 1.0f ),
	m_outerSphere( Vector3( 0.0f, 0.0f, 0.0f ), 1.0f )
{
}

EveSphereVolume::~EveSphereVolume()
{
}

void EveSphereVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const Color& baseColor )
{
	renderer.DrawSphere( this, TranslationMatrix( m_outerSphere.center ) * parentTransform, m_outerSphere.radius, 20, Tr2DebugRenderer::Wireframe, baseColor * 0.5f );
	renderer.DrawSphere( this, TranslationMatrix( m_outerSphere.center + m_innerSphere.center ) * parentTransform, m_innerSphere.radius, 20, Tr2DebugRenderer::Wireframe, baseColor * 0.5f );
}

const CcpMath::Sphere EveSphereVolume::GetBoundingSphere() const
{
	return m_outerSphere;
}

float EveSphereVolume::GetIntensity( Vector3 position )
{
	if( m_outerSphere.IsPointInside(position) )
	{
		// since the innersphere is offset from the outer sphere, we need to construct a new one that is centered in the owners object space
		CcpMath::Sphere innerModified(m_innerSphere);
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

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveSphereVolume::OnModified( Be::Var* val )
{
	m_innerSphere.radius = min( m_innerSphere.radius, m_outerSphere.radius );

	return true;
}