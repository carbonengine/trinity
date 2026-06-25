// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierBucket.h"


BLUE_DEFINE( EveSmartLightAttributeModifierBucket );

const Be::ClassInfo* EveSmartLightAttributeModifierBucket::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightAttributeModifierBucket, ":jessica-icon: bucket\n:jessica-icon-color: (112, 192, 29)\n" )
		MAP_INTERFACE( EveSmartLightAttributeModifierBucket )
		MAP_INTERFACE( IEveSmartLightGroupAttributeModifier )
		MAP_INTERFACE( EveSmartLightBaseAttributeModifier )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "organize your buckets", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "attributeModifiers", m_attributeModifiers, "list of attribute modifiers", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "attributeMultiplier", m_activationStrength, "scale modifier effect\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "active", m_active, "turn atrMod on/off\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "startsActive", m_startsActive, "should it be active on load (if false but active=true, then it will animate into active state)\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "crossFadeDuration", m_crossFadeDuration, "how many sec does the blending animation take\n:jessica-group: AttributeModifierBlending\n:jessica-numeric-range: (0.0,20.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "crossFadeIntensity", m_crossFadeIntensity, "is the animation linear or frontEnd/backend focused. (1=linear, closer to 0 comes in fast, above 1 -> ease in slowly)"
																   "\n:jessica-group: AttributeModifierBlending\n:jessica-numeric-range: (0.1,10.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "activationValue", m_activationValue, "[0:1] to visualize/debug the crossfade\n:jessica-group: AttributeModifierBlending", Be::READ )
		MAP_ATTRIBUTE( "finalAttributeMultiplier", m_finalActivationStrength, "to visualize/debug compound activationStrength\n:jessica-group: AttributeModifierBlending", Be::READ )

	EXPOSURE_END()
}
