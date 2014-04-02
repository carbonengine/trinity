////////////////////////////////////////////////////////////
//
//    Created:   October 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorCylinderLight.h"
#include "TriConstants.h"

BLUE_DEFINE( Tr2InteriorCylinderLight );


static Be::VarChooser DebugTypeChooser[] =
{
	{
		"WhiteVolumes",     
		BeCast( ITr2InteriorLight::DI_WHITE_VOLUMES ),     
		"Render light as white volume"
	},
	{
		"LightColor",     
		BeCast( ITr2InteriorLight::DI_LIGHT_COLOR ),     
		"Render light using the actual light source color"
	},
	{ 0 }
};

const Be::ClassInfo* Tr2InteriorCylinderLight::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorCylinderLight, "" )
        MAP_INTERFACE( Tr2InteriorCylinderLight )
        MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2InteriorLight )

		MAP_ATTRIBUTE( "name", m_name, "The name of this interior light source", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "position", m_position, "Position of the light", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Rotation of the light", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "radius", m_radius, "Cylinder radius", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "length", m_length, "Cylinder length", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "color", m_color, "The light's color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "kelvinColor", m_kelvinColor, "Kelvin color temperature", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useKelvinColor", m_useKelvinColor, "Use Kelvin color or RGB?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "Exponential falloff for distance from emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sectorAngleOuter", m_sectorAngleOuter, "Outer angle of cylinder sector", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "sectorAngleInner", m_sectorAngleInner, "Inner angle of cylinder sector", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "specularIntensity", m_specularIntensity, "Additional specular light intensity multiplier", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "secondaryLightingMultiplier", m_secondaryLightingMultiplier, "A factor to multiply into the contribution that this light makes to the radiosity", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "primaryLighting", m_primaryLighting, "Does this lightsource contribute to primary lighting?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "secondaryLighting", m_secondaryLighting, "Does this lightsource contribute to secondary lighting?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "affectTransparentObjects", m_affectTransparentObjects, "Does this lightsource affect transparent objects?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "renderDebugInfo", m_renderDebugInfo, "Render debug information", Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER( "renderDebugType", m_renderDebugType, "Type of debug visualization to use for light source (renderDebugInfo must be on)", Be::READWRITE | Be::ENUM, DebugTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectedTexturePath", m_projectedTexturePath, "The path used to load the projected texture map", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriTextureChooser )
		MAP_ATTRIBUTE( "projectedTextureRes", m_projectedTextureRes, "Projected texture map", Be::READ )
		MAP_METHOD_AND_WRAP( 
			"SetProjectedTexture", 
			SetProjectedTexture, 
			"Assign a projected texture to light source"
			"\n"
			"\nArguments:"
			"\ntexture - The TriTextureRes to set" );

		MAP_PROPERTY( "isStatic", IsStatic, SetStatic, "Should this light be treated as static by Enlighten?  Toggling forces a rebuild of cached input lighting!" )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boundingBox", m_boundingBox, "Additional bounding box for the light volume", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace", GetBoundingBoxInLocalSpace, "Gets the bounding box in local space" )
	EXPOSURE_END()
}

#endif
