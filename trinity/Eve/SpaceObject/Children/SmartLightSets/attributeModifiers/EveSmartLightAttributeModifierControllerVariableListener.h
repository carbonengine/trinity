// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "IEveSmartLightGroupAttributeModifier.h"
#include "EveSmartLightAttributeModifierBucket.h"

BLUE_CLASS( EveSmartLightAttributeModifierControllerVariableListener ) :
	public EveSmartLightAttributeModifierBucket
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightAttributeModifierControllerVariableListener( IRoot* lockobj = nullptr );

	void SetControllerVariable( const char* name, float value );
	bool OnModified( Be::Var * value ) override;

	// IInitialize
	bool Initialize() override;

private:
	std::string m_variableName;
	float m_value;
	float m_defaultValue;
	bool m_invertReceivedValue;
};

TYPEDEF_BLUECLASS( EveSmartLightAttributeModifierControllerVariableListener );