// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierControllerVariableListener.h"


BLUE_DEFINE( EveSmartLightAttributeModifierControllerVariableListener );

const Be::ClassInfo* EveSmartLightAttributeModifierControllerVariableListener::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightAttributeModifierControllerVariableListener, ":jessica-icon: computer-classic\n:jessica-icon-color: (112, 192, 29)\n" )
		MAP_INTERFACE( EveSmartLightAttributeModifierControllerVariableListener )
		MAP_INTERFACE( EveSmartLightAttributeModifierBucket )
		MAP_INTERFACE( IEveSmartLightGroupAttributeModifier )
		MAP_INTERFACE( EveSmartLightBaseAttributeModifier )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "attributeModifiers", m_attributeModifiers, "list of attribute modifiers", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "name", m_name, "organize your tree", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "variableName", m_variableName, "activate and deactivate bucket based on this variable", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "value", m_value, "editable for authoring but triggered automatically from controller states", Be::READWRITE | Be::PERSIST | Be::NOTIFY );
		MAP_ATTRIBUTE( "defaultValue", m_defaultValue, "if bucket should start active or not", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "invertReceivedValue", m_invertReceivedValue, "editable for authoring but triggered automatically from controller states", Be::READWRITE | Be::PERSIST | Be::NOTIFY );

		MAP_ATTRIBUTE( "attributeMultiplier", m_activationStrength, "scale modifier effect\n:jessica-group: AttributeModifierBlending", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "active", m_active, "triggered by controller events\n:jessica-group: AttributeModifierBlending", Be::READ )
		MAP_ATTRIBUTE( "crossFadeDuration", m_crossFadeDuration, "how many sec does the blending animation take\n:jessica-group: AttributeModifierBlending\n:jessica-numeric-range: (0.0,20.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "crossFadeIntensity", m_crossFadeIntensity, "is the animation linear or frontEnd/backend focused. (1=linear, closer to 0 comes in fast, above 1 -> ease in slowly)"
																   "\n:jessica-group: AttributeModifierBlending\n:jessica-numeric-range: (0.1,10.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "activationValue", m_activationValue, "[0:1] to visualize/debug the crossfade\n:jessica-group: AttributeModifierBlending", Be::READ )
		MAP_ATTRIBUTE( "finalAttributeMultiplier", m_finalActivationStrength, "to visualize/debug compound activationStrength\n:jessica-group: AttributeModifierBlending", Be::READ )

	EXPOSURE_END()
}
