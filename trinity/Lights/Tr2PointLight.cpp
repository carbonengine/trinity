// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Tr2PointLight.h"
#include <Tr2Renderer.h>


Tr2PointLight::Tr2PointLight( IRoot* lockobj ) :
	Tr2Light( lockobj )
{
	m_type = POINT_LIGHT;
}

void Tr2PointLight::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const Float4x3* bones, size_t boneCount )
{
	auto baseColor = m_lightData.color * m_lightData.brightness;
	baseColor.a = 0.1;
	auto selectedColor = baseColor + Color( 0.0, 0.0, 0.0, 0.2 );

	Matrix lightMatrix = m_boneTransform;
	if( m_lightData.boneIndex >= 0 && m_lightData.boneIndex < boneCount )
	{
		TriMatrixCopyFrom3x4( &lightMatrix, &bones[m_lightData.boneIndex] );
	}
	lightMatrix *= worldMatrix;

	renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.radius, 10, Tr2DebugRenderer::Wireframe, Tr2DebugColor( selectedColor, baseColor ) );
	renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.radius, 10, Tr2DebugRenderer::Solid, Color( 0, 0, 0, 0 ) );
	renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.innerRadius, 10, Tr2DebugRenderer::Wireframe, Tr2DebugColor( selectedColor, baseColor ) );
	renderer.DrawText( TRI_DBG_FONT_SMALL, TransformCoord( m_lightData.position, lightMatrix ), baseColor, "%s", m_name.empty() ? "Tr2PointLight" : m_name.c_str() );
}
