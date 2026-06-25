// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierExpressionBucket.h"


BLUE_DEFINE( EveSmartLightAttributeModifierExpressionBucket );

const Be::ClassInfo* EveSmartLightAttributeModifierExpressionBucket::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightAttributeModifierExpressionBucket, ":jessica-icon: square-root-variable\n:jessica-icon-color: (112, 192, 29)\n" )
		MAP_INTERFACE( EveSmartLightAttributeModifierExpressionBucket )
		MAP_INTERFACE( EveSmartLightAttributeModifierBucket )
		MAP_INTERFACE( IEveSmartLightGroupAttributeModifier )
		MAP_INTERFACE( EveSmartLightBaseAttributeModifier )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "organize your tree", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "expression", m_expression, "Curve expression\n:jessica-widget: expression", Be::PERSISTONLY )
		MAP_PROPERTY( "expression", GetExpression, SetExpression, "Curve expression\n:jessica-widget: expression" )

		MAP_ATTRIBUTE( "currentValue", m_activationStrength, "Value after the last update", Be::READ )

		MAP_ATTRIBUTE( "expressionInputs", m_inputs, "Scalar curve inputs", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "attributeModifiers", m_attributeModifiers, "list of attribute modifiers", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "finalAttributeMultiplier", m_finalActivationStrength, "to visualize/debug compound multiplier *affected by parent-strength(\n:jessica-group: AttributeModifierBlending", Be::READ )

	EXPOSURE_END()
}
