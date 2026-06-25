// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "IEveSmartLightGroupAttributeModifier.h"
#include "EveSmartLightAttributeModifierBucket.h"

BLUE_CLASS( EveSmartLightAttributeModifierCameraDependency ) :
	public EveSmartLightAttributeModifierBucket
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightAttributeModifierCameraDependency( IRoot* lockobj = nullptr );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier );
	void ProcessAttributeModifier( Vector3 & attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength ) override;

private:
	const float GetActivationValue( const Vector3& objectPosition, const Vector3& entityDirection );
	const float GetDistanceAmplitude( const Vector3& vec2Obj );
	const float GetLookAtAmplitude( const Vector3& vec2obj );
	const float GetPlacementAmplitude( const Vector3& vec2Obj, const Vector3& entityDirection );

	bool m_useCameraDistance;
	bool m_inverseDistanceFormula;
	bool m_useCameraLookAt;
	bool m_inverselookAtFormula;
	bool m_useCameraPlacement;
	bool m_inversePlacementFormula;
	bool m_overwritePosition;
	bool m_overwriteDirection;

	float m_minimumActivation;
	float m_maximumActivation;
	float m_minimumDistance;
	float m_maximumDistance;
	float m_lookAtIntencity;
	float m_lookAtVisionCone;
	float m_placementIntencity;

	Vector3 m_positionOverwrite;
	Vector3 m_angleOverwrite;
};

TYPEDEF_BLUECLASS( EveSmartLightAttributeModifierCameraDependency );
