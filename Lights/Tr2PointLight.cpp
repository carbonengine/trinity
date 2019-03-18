////////////////////////////////////////////////////////////
//
//    Created:   January 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2PointLight.h"


Tr2PointLight::Tr2PointLight( IRoot* lockobj ):
	Tr2Light( lockobj )
{
	m_type = POINT_LIGHT;
}

void Tr2PointLight::RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& worldMatrix ) 
{
	auto baseColor = m_lightData.color * m_lightData.brightness;
	baseColor.a = 0.1;
	auto selectedColor = baseColor + Color( 0.0, 0.0, 0.0, 0.2 );
	
	renderer.DrawSphere( this, worldMatrix, m_lightData.position, m_lightData.radius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
	renderer.DrawSphere( this, worldMatrix, m_lightData.position, m_lightData.innerRadius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor) );
}

