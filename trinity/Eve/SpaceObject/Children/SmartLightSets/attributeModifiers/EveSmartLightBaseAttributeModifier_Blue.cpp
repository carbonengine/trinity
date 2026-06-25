// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightBaseAttributeModifier.h"

const Be::VarChooser LifetimeFormulaChooser[] = {
	{ "Per Instance Lifetime", BeCast( EveSmartLightBaseAttributeModifier::PER_INSTANCE_LIFETIME ), "animate each object individually" },
	{ "Modifier Play Time", BeCast( EveSmartLightBaseAttributeModifier::PER_MODIFIER_PLAYTIME ), "simultanously animate all things in group" },
	{ "Static", BeCast( EveSmartLightBaseAttributeModifier::STATIC ), "not animated, only use curve for per instance offset values" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "LifetimeFormula", DistributionEntityLifeTimeEvent, LifetimeFormulaChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE_INTERFACE( IEveSmartLightGroupAttributeModifier );

BLUE_DEFINE( EveSmartLightBaseAttributeModifier );

const Be::ClassInfo* EveSmartLightBaseAttributeModifier::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightBaseAttributeModifier, "" )
		MAP_INTERFACE( EveSmartLightBaseAttributeModifier )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "attributeMultiplier", m_activationStrength, "scale modifier effect\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "active", m_active, "turn the attribute modifier on/off\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "activationOverLifetime", m_activationOverLifetime, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "lifeTimeFormula", m_lifeTimeFormula, "how to calculate lifetime\n:jessica-group: LifeTimeValueSettings", Be::READWRITE | Be::PERSIST | Be::ENUM, LifetimeFormulaChooser );
		MAP_ATTRIBUTE( "perInstanceOffset", m_perInstanceOffset, "offest based on id*this on the lifeTime curve\n:jessica-group: LifeTimeValueSettings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "restartPlayTimeWhenInactive", m_restartPlayTimeWhenInactive, "to be able to trigger playTime start sequences multiple times \n:jessica-group: LifeTimeValueSettings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "playTime", m_playTime, "for how long modifier has been running \n:jessica-group: LifeTimeValueSettings", Be::READ )

		MAP_ATTRIBUTE( "startsActive", m_startsActive, "should it be active on load (if false but active=true, then it will animate into active state)\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "delayedActivation", m_delayedActivation, "when triggered, should we wait before activating?\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "crossFadeDuration", m_crossFadeDuration, "how many sec does the blending animation take\n:jessica-group: AttributeModifierBlending\n:jessica-numeric-range: (0.0,20.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "crossFadeIntensity", m_crossFadeIntensity, "is the animation linear or frontEnd/backend focused. (1=linear, closer to 0 comes in fast, above 1 -> ease in slowly)"
																   "\n:jessica-group: AttributeModifierBlending\n:jessica-numeric-range: (0.1,10.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "activationValue", m_activationValue, "[0:1] to visualize/debug the crossfade\n:jessica-group: AttributeModifierBlending", Be::READ )
		MAP_ATTRIBUTE( "finalAttributeMultiplier", m_finalActivationStrength, "to visualize/debug compound multiplier *affected by parent-strength(\n:jessica-group: AttributeModifierBlending", Be::READ )

	EXPOSURE_END()
}
