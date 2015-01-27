////////////////////////////////////////////////////////////
//
//    Created:   October 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#include "Tr2InteriorCylinderLight.h"
#include "TriConstants.h"

BLUE_DEFINE( Tr2InteriorCylinderLight );

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
		MAP_ATTRIBUTE( "primaryLighting", m_primaryLighting, "Does this lightsource contribute to primary lighting?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "affectTransparentObjects", m_affectTransparentObjects, "Does this lightsource affect transparent objects?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectedTexturePath", m_projectedTexturePath, "The path used to load the projected texture map", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriTextureChooser )
		MAP_ATTRIBUTE( "projectedTextureRes", m_projectedTextureRes, "Projected texture map", Be::READ )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace", GetBoundingBoxInLocalSpace, "Gets the bounding box in local space" )
	EXPOSURE_END()
}

