// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "IEveSmartLightGroupAttributeModifier.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

BLUE_CLASS( EveSmartLightAttributeModifierColor ) :
	public EveSmartLightBaseAttributeModifier,
	public IEveSmartLightGroupAttributeModifier
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightAttributeModifierColor( IRoot* lockobj = nullptr );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier );
	void ProcessAttributeModifier( Vector3 & attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength ) override;
	void SetInheritProperties( const Color* colorSet ) override;

private:
	Color GetGroupColor() const;

	bool m_useFactionColor;

	float m_saturationMultiplier;
	float m_brightnessMultiplier;
	float m_blendValue;
	Color m_blendColor;
	const Color* m_parentColorSet;
	int32_t m_selectedColor;
};

TYPEDEF_BLUECLASS( EveSmartLightAttributeModifierColor );
