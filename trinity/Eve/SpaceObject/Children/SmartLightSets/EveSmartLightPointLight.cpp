#include "StdAfx.h"
#include "EveSmartLightPointLight.h"
#include "Resources/Tr2LightProfileRes.h"
#include "TriMath.h"

EveSmartLightPointLight::EveSmartLightPointLight( IRoot* lockobj ):
	m_staticOffsetTranslation( 0.f, 0.f, 0.f),
	m_staticOffsetRotation( 0.f, 0.f, 0.f, 1.f ),
	m_activationStrength( 1.f ),
	m_display( true ),
	m_distribution( nullptr )
{
	m_lightGroupData = LightData();
	m_lightType = Tr2Light::POINT_LIGHT;
}

bool EveSmartLightPointLight::Initialize()
{
	Tr2LightProfileResPtr profile;
	if( !m_lightProfilePath.empty() )
	{
		BeResMan->GetResource( m_lightProfilePath, L"lp", profile );
	}
	m_lightProfile = profile;
	return true;
}

bool EveSmartLightPointLight::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_lightProfilePath ) )
	{
		Tr2LightProfileResPtr profile;
		if( !m_lightProfilePath.empty() )
		{
			BeResMan->GetResource( m_lightProfilePath, L"lp", profile );
		}
		m_lightProfile = profile;
	}
	return true;
}

void EveSmartLightPointLight::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	m_activationStrength = params.activationStrength;
	m_worldTransform = params.localToWorldTransform;

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->UpdateSyncronous( updateContext, params, 1.f );
	}

	m_distribution = distribution;
}

void EveSmartLightPointLight::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )	
	{
		registry->RegisterComponent<ITr2LightOwner>( this );
	}
}

void EveSmartLightPointLight::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}

	const PlacementDataWithIdentifierStructureList& placements = *m_distribution->GetPlacementData();
	size_t size = m_distribution->GetNumberOfPlacements();

	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetX() ),
											   XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetY() ), 
											   XMVector3LengthEst( m_worldTransform.GetZ() ) ) ) ) / 3.f;
	int16_t profileIndex = m_lightProfile ? m_lightProfile->GetTextureIndex() + 1 : 0;
	Vector4 groupColor = this->GetGroupColor();

	for( size_t index = 0; index < size; index++ )
	{
		XMMATRIX lightTransform = m_worldTransform;
		Vector3 lightPosition( 0.f, 0.f, 0.f );
		Vector3 lightRotation( 0.f, 1.f, 0.f );
		Tr2LightManager::PerLightData pointLightData;
			
		float perLightScaling = std::max( { placements[index].initialScale.x, placements[index].initialScale.y , placements[index].initialScale.z } );
		perLightScaling *= std::max( { placements[index].additionalScale.x, placements[index].additionalScale.y, placements[index].additionalScale.z } );

		pointLightData.radius = m_lightGroupData.radius * scaling * perLightScaling;
		pointLightData.innerRadius = Float_16( m_lightGroupData.innerRadius * scaling * perLightScaling );

		pointLightData.flags = m_lightGroupData.flags | ( profileIndex << 4 );

		Quaternion rotation = placements[index].initialRotation * placements[index].additionalRotation;
		if( m_staticOffsetTranslation != Vector3( 0.f, 0.f, 0.f ) )
		{
			TriVectorRotateQuaternion( &lightPosition, &m_staticOffsetTranslation, &rotation );
		}

		lightPosition += placements[index].initialTranslation + placements[index].additionalTranslation;
		lightPosition = Vector3( XMVector3TransformCoord( lightPosition, lightTransform ) );
		pointLightData.position = lightPosition;
		
		rotation *= m_staticOffsetRotation;
		TriVectorRotateQuaternion( &lightRotation, &lightRotation, &rotation );
		lightRotation *= -1.f;
		TriVectorRotateMatrix( &lightRotation, &lightRotation, &m_worldTransform );
		pointLightData.direction = Vector3_16( Normalize( lightRotation ) );

		pointLightData.color = ( groupColor * m_lightGroupData.brightness * m_activationStrength ).GetXYZ();

		for( auto attributeModifier : m_attributeModifiers )
		{
			attributeModifier->ProcessAttributeModifier( pointLightData.color, placements[index], lightPosition, lightRotation, m_activationStrength );
		}

		pointLightData.outerAngle = Float_16( 0.0f );
		pointLightData.innerAngle = Float_16( 0.0f );

		if( m_lightType == Tr2Light::SPOT_LIGHT )
		{
			pointLightData.outerAngle = Float_16( cos( TRI_2PI * m_lightGroupData.outerAngle / 360.0f ) );
			pointLightData.innerAngle = Float_16( cos( TRI_2PI * m_lightGroupData.innerAngle / 360.0f ) );
		}
		
		lightManager.AddLight( pointLightData );
	}
}

void EveSmartLightPointLight::RenderDebugInfo( ITr2DebugRenderer2& renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size )
{
	if( !renderer.HasOption( this, "smartLightSets" ) || !m_display )
	{
		return;
	}

	auto baseColor = this->GetGroupColor() * m_lightGroupData.brightness;
	baseColor.a = 0.03;
	auto selectedColor = baseColor * 1.5f + Color( 0.0, 0.0, 0.0, 0.01 );
	XMMATRIX lightTransform = m_worldTransform;

	for( size_t index = 0; index < size; index++ )
	{
		Vector3 ligtPosition;
		Quaternion lightRotation = placements[index].initialRotation * placements[index].additionalRotation;
		TriVectorRotateQuaternion( &ligtPosition, &m_staticOffsetTranslation, &lightRotation );
		ligtPosition += placements[index].initialTranslation + placements[index].additionalTranslation;

		float perLightScaling = std::max( { placements[index].initialScale.x, placements[index].initialScale.y, placements[index].initialScale.z } );
		perLightScaling *= std::max( { placements[index].additionalScale.x, placements[index].additionalScale.y, placements[index].additionalScale.z } );

		float radius = m_lightGroupData.radius * perLightScaling;
		float innerRadius = Float_16( m_lightGroupData.innerRadius * perLightScaling );

		renderer.DrawSphere( this, lightTransform, ligtPosition, radius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
		renderer.DrawSphere( this, lightTransform, ligtPosition, innerRadius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
	}
}
