// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierCameraDependency.h"
#include "TriMath.h"
#include "Tr2Renderer.h"

EveSmartLightAttributeModifierCameraDependency::EveSmartLightAttributeModifierCameraDependency( IRoot* lockobj ) :
	EveSmartLightAttributeModifierBucket( lockobj ),
	m_minimumActivation( 0.f ),
	m_maximumActivation( 1.f ),
	m_useCameraDistance( false ),
	m_inverseDistanceFormula( false ),
	m_minimumDistance( 1000.f ),
	m_maximumDistance( 10000.f ),
	m_useCameraLookAt( false ),
	m_inverselookAtFormula( false ),
	m_lookAtIntencity( 1.f ),
	m_lookAtVisionCone( 30.f ),
	m_useCameraPlacement( false ),
	m_inversePlacementFormula( false ),
	m_placementIntencity( 1.f ),
	m_overwritePosition( false ),
	m_overwriteDirection( false ),
	m_positionOverwrite( 0.f, 0.f, 0.f ),
	m_angleOverwrite( 0.f, 0.f, 0.f )
{
	m_name = "CameraDependency";
}

void EveSmartLightAttributeModifierCameraDependency::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier )
{
	this->UpdateActivationStrength( activationMultiplier, updateContext.GetDeltaT() );
}

const float EveSmartLightAttributeModifierCameraDependency::GetDistanceAmplitude( const Vector3& vec2obj )
{
	if( m_useCameraDistance )
	{
		float distance = Length( vec2obj );
		float distanceAmplitude = min( 1.f, max( 0.f, ( distance - m_minimumDistance ) / ( m_maximumDistance - m_minimumDistance ) ) );

		if( m_inverseDistanceFormula )
		{
			distanceAmplitude = 1.f - distanceAmplitude;
		}
		return distanceAmplitude;
	}
	return 1.f;
}

const float EveSmartLightAttributeModifierCameraDependency::GetLookAtAmplitude( const Vector3& vec2obj )
{
	if( m_useCameraLookAt )
	{
		float lookAtAmplitude = -( Dot( Tr2Renderer::GetViewLookAt(), Normalize( vec2obj ) ) );
		if( m_lookAtVisionCone < 90.f && m_lookAtVisionCone > 0.f )
		{
			float startValue = ( 90.f - m_lookAtVisionCone ) / 90.f;
			lookAtAmplitude = max( lookAtAmplitude - startValue, 0.f ) / ( 1.f - startValue );
		}

		lookAtAmplitude = pow( lookAtAmplitude, m_lookAtIntencity );

		if( m_inverselookAtFormula )
		{
			lookAtAmplitude = 1.f - lookAtAmplitude;
		}
		return lookAtAmplitude;
	}
	return 1.f;
}

const float EveSmartLightAttributeModifierCameraDependency::GetPlacementAmplitude( const Vector3& vec2obj, const Vector3& entityDirection )
{
	if( m_useCameraPlacement )
	{
		Vector3 eDir = m_overwriteDirection ? Normalize( m_angleOverwrite ) : entityDirection;
		float placementAmplitude = max( 0.f, -Dot( Normalize( vec2obj ), eDir ) );

		if( m_placementIntencity != 1.f )
		{
			placementAmplitude = pow( placementAmplitude, m_placementIntencity );
		}

		if( m_inversePlacementFormula )
		{
			placementAmplitude = 1.f - placementAmplitude;
		}
		return placementAmplitude;
	}
	return 1.f;
}

const float EveSmartLightAttributeModifierCameraDependency::GetActivationValue( const Vector3& objectPosition, const Vector3& entityDirection )
{
	const Vector3 camPos = Tr2Renderer::GetViewPosition();
	const Vector3 vec2obj = m_overwritePosition ? m_positionOverwrite - camPos : objectPosition - camPos;
	float activationValue = 1.f;
	activationValue *= GetDistanceAmplitude( vec2obj );
	activationValue *= GetLookAtAmplitude( vec2obj );
	activationValue *= GetPlacementAmplitude( vec2obj, entityDirection );
	return Lerp( m_minimumActivation, m_maximumActivation, activationValue );
}

void EveSmartLightAttributeModifierCameraDependency::ProcessAttributeModifier( Vector3& attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength )
{
	float activationValue = this->GetActivationValue( entityPosition, entityDirection );

	if( activationValue != 0.f )
	{
		for( auto attributeModifier : m_attributeModifiers )
		{
			attributeModifier->ProcessAttributeModifier( attribute, placement, entityPosition, entityDirection, activationValue * modifierStrength * m_activationStrength );
		}
	}
}
