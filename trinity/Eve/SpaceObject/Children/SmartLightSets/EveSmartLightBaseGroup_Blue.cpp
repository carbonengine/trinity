// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightBaseGroup.h"

BLUE_DEFINE( EveSmartLightBaseGroup );

const Be::ClassInfo* EveSmartLightBaseGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightBaseGroup, ":jessica-deprecated: True" )
		MAP_INTERFACE( EveSmartLightBaseGroup )
		MAP_INTERFACE( IEveSmartLightGroup )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "useFactionColor", m_useFactionColor, "if checked it will use factionColorToBlend\n:jessica-group: ColorSettings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "factionColor", m_selectedColor, "Light color\n:jessica-group: ColorSettings", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "customColor", m_color, "Quad color\n:jessica-tuple-type: linearcolor\n:jessica-group: ColorSettings", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "activeColor", GetGroupColor, "color being used\n:jessica-tuple-type: linearcolor\n:jessica-group: ColorSettings" )

		MAP_ATTRIBUTE( "attributeModifiers", m_attributeModifiers, "list of attribute modifiers", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
