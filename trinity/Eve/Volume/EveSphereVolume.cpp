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
	m_position( 0, 0, 0 ),
	m_centerOffset( 0, 0, 0 ),
	m_radius( 0 ),
	m_innerRadius( 0 ),
	m_notifyParent( false )
{
}

EveSphereVolume::~EveSphereVolume()
{
}

void EveSphereVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform )
{
	renderer.DrawSphere( this, TranslationMatrix( m_position ) * parentTransform, m_radius, 20, Tr2DebugRenderer::Wireframe, 0xff555555 );
	renderer.DrawSphere( this, TranslationMatrix( m_position + m_centerOffset ) * parentTransform, max( m_innerRadius, 1.0f ), 20, Tr2DebugRenderer::Wireframe, 0xff777777 );
}

Vector4 EveSphereVolume::GetBoundingSphere() const
{
	return Vector4( m_position, m_radius );
}

float EveSphereVolume::GetIntensity( Vector3 position )
{
	// are we inside of the outer radius?
	float radiusSq = m_radius * m_radius;
	if (LengthSq( position - m_position ) > radiusSq)
	{
		return 0;
	}

	Vector3 line = position - m_position - m_centerOffset;
	float distanceToInnerRadiusSq = LengthSq( line );
	float innerRadiusSq = m_innerRadius * m_innerRadius;
	if (distanceToInnerRadiusSq <  innerRadiusSq)
	{
		return 1;
	}

	return 1.0f - distanceToInnerRadiusSq / (radiusSq - innerRadiusSq);
	/*
	// TODO make math good!

	// find the intersection line between the camera position and the center offset
	// determening the intensity on how far we are from that line to the inner radius
	line = m_centerOffset - cameraPosition;
	Vector3 centerToOffset = m_centerOffset - m_position;

	// project the center to the line
	Vector3 centerProj = m_centerOffset + Dot( centerToOffset, line ) / Dot( line, line ) * line;

	//
	//   Cam --x----- CO
	//         |
	//         |
	//         P
	//

	float centerProjToBoundsSq = radiusSq - LengthSq( centerProj - m_position );

	float cameraPosToCenterProjSq = LengthSq( centerProj - cameraPosition );
	float centerOffsetToCenterProjSq = LengthSq( centerProj - m_centerOffset ) - innerRadiusSq;

	float totalDistance = centerOffsetToCenterProjSq + centerProjToBoundsSq;
	float cameraDistance = LengthSq(cameraPosition - m_centerOffset) - innerRadiusSq;

	return 1.0f - cameraDistance / totalDistance;
	*/
}

void EveSphereVolume::RegisterForChanges( std::function<void()> NotifyParent )
{
	m_notifyParentFunc = NotifyParent;
	m_notifyParent = true;
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveSphereVolume::OnModified( Be::Var* val )
{
	m_innerRadius = min( m_innerRadius, m_radius );

	if (m_notifyParent && (IsMatch( val, m_position ) || IsMatch( val, m_radius )))
	{
		m_notifyParentFunc();
	}
	return true;
}