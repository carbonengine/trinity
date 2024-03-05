////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2SpotLight.h"
#include "Tr2DebugRenderer.h"

#include "Tr2LightManager.h"
#include "Include/TriMath.h"

Tr2SpotLight::Tr2SpotLight( IRoot* lockobj ):
	Tr2Light( lockobj )
{
	m_type = SPOT_LIGHT;
}

void Tr2SpotLight::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const granny_matrix_3x4* bones, size_t boneCount  )
{
	auto baseColor = m_lightData.color * m_lightData.brightness;
	baseColor.a = 0.025f;
	auto colorMod = Color( 0.0f, 0.0f, 0.0f, 0.025f );

	Matrix boneMatrix = m_boneTransform;
	if( m_lightData.boneIndex >= 0 && m_lightData.boneIndex < boneCount ) {
		TriMatrixCopyFrom3x4( &boneMatrix, &bones[m_lightData.boneIndex] );
	}

	Matrix lightMatrix = RotationMatrix(m_lightData.rotation) * TranslationMatrix(m_lightData.position) * boneMatrix * worldMatrix;	

	float outerAngle = TRI_2PI * m_lightData.outerAngle / 360.f;
	float innerAngle = TRI_2PI * m_lightData.innerAngle / 360.f;

	renderer.DrawCone( this, lightMatrix, m_lightData.radius, outerAngle, 15, 15, Tr2DebugRenderer::Solid, Tr2DebugColor( baseColor + colorMod * 2.0f, baseColor ) );
	renderer.DrawCone( this, lightMatrix, m_lightData.innerRadius, innerAngle, 15, 15, Tr2DebugRenderer::Solid, Tr2DebugColor( baseColor + colorMod * 3.0f, baseColor + colorMod  ) );
}