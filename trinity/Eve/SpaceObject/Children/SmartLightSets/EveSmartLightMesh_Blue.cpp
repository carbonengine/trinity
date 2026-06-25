// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightMesh.h"

BLUE_DEFINE( EveSmartLightMesh );

extern Be::VarChooser RotationalConstraintsChooser[];

const Be::ClassInfo* EveSmartLightMesh::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightMesh, ":jessica-icon: ufo\n" )
		MAP_INTERFACE( EveSmartLightMesh )
		MAP_INTERFACE( EveChildInstanceMeshRenderer )
		MAP_INTERFACE( IEveSmartLightGroup )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveShadowCaster )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "mesh", m_mesh, "the rendered mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "castShadows", m_castShadow, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER( "rotationConstraint", m_rotationConstraint, "", Be::READWRITE | Be::PERSIST | Be::ENUM, RotationalConstraintsChooser );

		MAP_ATTRIBUTE( "staticOffsetTranslation", m_staticOffsetTranslation, "static per instance offset in local space before placement", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticOffsetRotation", m_staticOffsetRotation, "local per instance rotation to adjust meshes that are turning the wrong way by default", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticOffsetScale", m_staticOffsetScale, "to reuse smaller or larger assets (or deform them) and have them fit with where they are distributed", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "shaderParamColorName", m_shaderParamColorName, "if configured, this shader parameters is set during runtime to colorValue\n:jessica-group: ShaderColorOverride", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useFactionColor", m_useFactionColor, "if checked it will use factionColorToBlend\n:jessica-group: ShaderColorOverride", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "factionColor", m_selectedColor, "Light color\n:jessica-group: ShaderColorOverride", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "customColor", m_color, "Quad color\n:jessica-tuple-type: linearcolor\n:jessica-group: ShaderColorOverride", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "minScreenSize", m_minScreenSize, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "", Be::READ )

		MAP_ATTRIBUTE( "attributeModifiers", m_attributeModifiers, "list of attribute modifiers", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "RefreshStaticGeometry", RefreshStaticGeometry, "if static geo parameters were changed during authoring: refresh here\n:jessica-placement: TOOLBAR" )

	EXPOSURE_END()
}
