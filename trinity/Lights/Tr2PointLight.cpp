////////////////////////////////////////////////////////////
//
//    Created:   January 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2PointLight.h"
#include <Tr2Renderer.h>


Tr2PointLight::Tr2PointLight( IRoot* lockobj ) :
	Tr2Light( lockobj ),
	m_blinks( false ),
	m_blinkRate( 0.0 ),
	m_blinkPhase( 0.0f ),
	m_minScale( 0.0f ),
	m_maxScale( 0.0f ),
	m_originalInnerRadius( 0.0f ),
	m_originalRadius( 0.0f )
{
	m_type = POINT_LIGHT;
}

void Tr2PointLight::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto baseColor = m_lightData.color * m_lightData.brightness;
	baseColor.a = 0.1;
	auto selectedColor = baseColor + Color( 0.0, 0.0, 0.0, 0.2 );

	Matrix lightMatrix = m_boneTransform;
	if( m_lightData.boneIndex >= 0 && m_lightData.boneIndex < boneCount ) {
		TriMatrixCopyFrom3x4( &lightMatrix, &bones[m_lightData.boneIndex] );
	}
	lightMatrix *= worldMatrix;

	renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.radius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
	renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.innerRadius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
}

void Tr2PointLight::SetBlinkingBehavior( float blinkRate, float blinkPhase, float minScale, float maxScale ) {

	m_blinks = true;
	m_isDynamic = true;
	m_blinkRate = blinkRate;
	m_blinkPhase = blinkPhase;
	m_minScale = minScale;
	m_maxScale = maxScale;
	// store the original scale so we can resize it when we "blink"
	m_originalRadius = m_lightData.radius;
	m_originalInnerRadius = m_lightData.innerRadius;
}

void Tr2PointLight::Update() {
	if( m_blinks )
	{
		auto blinkScale = ( m_maxScale - m_minScale ) * Blink() + m_minScale;
		m_lightData.radius = m_originalRadius * blinkScale;
		m_lightData.innerRadius = m_originalInnerRadius * blinkScale;
	}
}

float Tr2PointLight::Blink() const
{
	const float FLASH_PEAK_TIME = 0.05f;
	float intPart;
	float f = modf(Tr2Renderer::GetAnimationTime() * m_blinkRate + m_blinkPhase, &intPart);

	float peak = FLASH_PEAK_TIME * m_blinkRate;
	float result = 0.0f;
	float end = peak * 4.0f;

	auto lerp = [] ( float a, float b, float f ) {
		return a + f * ( b - a );
		};

	if( peak < 0.0001f )
	{
		peak = 1.0f;
	}
	if( f < peak )
	{
		result = lerp( 0.0f, 1.0f, f / peak );
	}
	else if( f < end )
	{
		result = lerp( 1.0f, 0.0f, ( f - peak ) / ( end - peak ) );
	}

	return result;
}


