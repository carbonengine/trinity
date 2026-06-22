// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierColor.h"


BLUE_DEFINE( EveSmartLightAttributeModifierColor );

const Be::ClassInfo* EveSmartLightAttributeModifierColor::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightAttributeModifierColor, ":jessica-icon: paint-roller\n:jessica-icon-color: (255, 115, 35)\n" )
		MAP_INTERFACE( EveSmartLightAttributeModifierColor )
		MAP_INTERFACE( IEveSmartLightGroupAttributeModifier )

		MAP_ATTRIBUTE( "name", m_name, "organize your buckets", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "brightnessMultiplier", m_brightnessMultiplier, "scale saturation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "saturationMultiplier", m_saturationMultiplier, "scale saturation", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "blendValue", m_blendValue, "0 -> don't mix colors, [0:1]->mix, 1-> use override\n:jessica-numeric-range: (0.0,1.0)\n:jessica-group: ColorModifications", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useFactionColor", m_useFactionColor, "if checked it will use factionColorToBlend\n:jessica-group: ColorModifications", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "factionColor", m_selectedColor, "Light color\n:jessica-group: ColorModifications", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "blendColor", m_blendColor, "override color\n:jessica-tuple-type: linearcolor\n:jessica-group: ColorModifications", Be::READWRITE | Be::PERSIST )

		MAP_PROPERTY_READONLY( "activeBlendColor", GetGroupColor, "color being used\n:jessica-tuple-type: linearcolor\n:jessica-group: ColorModifications" )

	EXPOSURE_CHAINTO( EveSmartLightBaseAttributeModifier )
}
