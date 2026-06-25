// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2FactionLight.h"


Tr2FactionLight::Tr2FactionLight( IRoot* lockobj ) :
	Tr2Light( lockobj ),
	m_selectedColor( -1 ),
	m_isSpotlight( false ),
	m_parentColorSet( nullptr ),
	m_saturation( 1.0f )
{
	m_type = POINT_LIGHT; // POINT_LIGHT or SPOT_LIGHT
}

bool Tr2FactionLight::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_isSpotlight ) )
	{
		m_isSpotlight ? m_type = SPOT_LIGHT : m_type = POINT_LIGHT;
	}

	if( IsMatch( value, m_selectedColor ) )
	{
		SetLightColorFromFactionColor();
	}

	if( IsMatch( value, m_saturation ) )
	{
		SetLightColorFromFactionColor();
	}

	return Tr2Light::OnModified( value );
}

void Tr2FactionLight::SetLightColorFromFactionColor()
{
	if( nullptr == m_parentColorSet )
	{
		return;
	}

	if( m_selectedColor >= 0 && m_selectedColor < SOFDataFactionColorChooser::TYPE_MAX )
	{
		m_lightData.color = Saturate( m_parentColorSet[m_selectedColor], m_saturation );
	}
}

void Tr2FactionLight::SetInheritProperties( const Color* colorSet )
{
	if( colorSet )
	{
		m_parentColorSet = colorSet;
		SetLightColorFromFactionColor();
	}
}

Color Tr2FactionLight::GetSelectedColor() const
{
	return m_lightData.color;
}

void Tr2FactionLight::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const Float4x3* bones, size_t boneCount )
{
	if( m_isSpotlight )
	{
		auto baseColor = m_lightData.color * m_lightData.brightness;
		baseColor.a = 0.025f;
		auto colorMod = Color( 0.0f, 0.0f, 0.0f, 0.025f );

		Matrix boneMatrix = m_boneTransform;
		if( m_lightData.boneIndex >= 0 && m_lightData.boneIndex < boneCount )
		{
			TriMatrixCopyFrom3x4( &boneMatrix, &bones[m_lightData.boneIndex] );
		}

		Matrix lightMatrix = RotationMatrix( m_lightData.rotation ) * TranslationMatrix( m_lightData.position ) * boneMatrix * worldMatrix;

		float outerAngle = TRI_2PI * m_lightData.outerAngle / 360.f;
		float innerAngle = TRI_2PI * m_lightData.innerAngle / 360.f;

		renderer.DrawCone( this, lightMatrix, m_lightData.radius, outerAngle, 15, 15, Tr2DebugRenderer::Solid, Tr2DebugColor( baseColor + colorMod * 2.0f, baseColor ) );
		renderer.DrawCone( this, lightMatrix, m_lightData.innerRadius, innerAngle, 15, 15, Tr2DebugRenderer::Solid, Tr2DebugColor( baseColor + colorMod * 3.0f, baseColor + colorMod ) );
	}
	else
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

		renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.radius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
		renderer.DrawSphere( this, lightMatrix, m_lightData.position, m_lightData.innerRadius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
	}
}
