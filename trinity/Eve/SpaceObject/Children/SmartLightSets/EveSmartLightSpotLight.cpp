// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightSpotLight.h"
#include "TriMath.h"

EveSmartLightSpotLight::EveSmartLightSpotLight( IRoot* lockobj ) :
	EveSmartLightPointLight( lockobj )
{
	m_lightType = Tr2Light::SPOT_LIGHT;
}

void EveSmartLightSpotLight::RenderDebugInfo( ITr2DebugRenderer2& renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size )
{
	if( !renderer.HasOption( this, "smartLightSets" ) || !m_display )
	{
		return;
	}

	auto baseColor = this->GetGroupColor() * m_lightGroupData.brightness;
	baseColor.a = 0.03;
	auto colorMod = Color( 0.0f, 0.0f, 0.0f, 0.025f );

	for( size_t index = 0; index < size; index++ )
	{
		float perLightScaling = max( max( placements[index].initialScale.x, placements[index].initialScale.y ), placements[index].initialScale.z );
		perLightScaling *= max( max( placements[index].additionalScale.x, placements[index].additionalScale.y ), placements[index].additionalScale.z );

		float radius = m_lightGroupData.radius * perLightScaling;
		float innerRadius = Float_16( m_lightGroupData.innerRadius * perLightScaling );

		float outerAngle = TRI_2PI * m_lightGroupData.outerAngle / 360.f;
		float innerAngle = TRI_2PI * m_lightGroupData.innerAngle / 360.f;

		Vector3 ligtPosition( 0.f, 0.f, 0.f );
		Quaternion rotation = placements[index].initialRotation * placements[index].additionalRotation;

		if( m_staticOffsetTranslation != ligtPosition )
		{
			TriVectorRotateQuaternion( &ligtPosition, &m_staticOffsetTranslation, &rotation );
		}

		ligtPosition += placements[index].initialTranslation + placements[index].additionalTranslation;

		Vector3 lightRotation( 0.f, 1.f, 0.f );
		Quaternion rot;
		TriVectorRotateQuaternion( &lightRotation, &lightRotation, &rotation );
		lightRotation = Normalize( lightRotation ) * -1.f;
		TriQuaternionArcFromForward( &rot, &lightRotation );
		rot *= m_staticOffsetRotation;
		Matrix lightMatrix = RotationMatrix( rot ) * TranslationMatrix( ligtPosition ) * m_worldTransform;

		renderer.DrawCone( this, lightMatrix, radius, outerAngle, 15, 15, Tr2DebugRenderer::Solid, Tr2DebugColor( baseColor + colorMod * 2.0f, baseColor ) );
		renderer.DrawCone( this, lightMatrix, innerRadius, innerAngle, 15, 15, Tr2DebugRenderer::Solid, Tr2DebugColor( baseColor + colorMod * 3.0f, baseColor + colorMod ) );
	}
}
