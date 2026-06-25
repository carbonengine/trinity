// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "IEveSmartLightGroupAttributeModifier.h"

BLUE_CLASS( EveSmartLightAttributeModifierNoise ) :
	public EveSmartLightBaseAttributeModifier,
	public IEveSmartLightGroupAttributeModifier
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightAttributeModifierNoise( IRoot* lockobj = nullptr );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier ) override;
	void ProcessAttributeModifier( Vector3 & attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength ) override;

private:
	float m_noiseAmplitude;
	float m_noiseFrequency;
	uint32_t m_noiseOctaves;
};

TYPEDEF_BLUECLASS( EveSmartLightAttributeModifierNoise );
