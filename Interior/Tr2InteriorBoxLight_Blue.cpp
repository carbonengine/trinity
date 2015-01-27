#include "StdAfx.h"

#include "Tr2InteriorBoxLight.h"
#include "TriConstants.h"

BLUE_DEFINE( Tr2InteriorBoxLight );


static Be::VarChooser ShadowResolutionChooser[] =
{
	{
		"32",     
		BeCast( 32 ),     
		"32"
	},
	{
		"64",     
		BeCast( 64 ),     
		"64"
	},
	{
		"128",     
		BeCast( 128 ),     
		"128"
	},
	{
		"256",     
		BeCast( 256 ),     
		"256"
	},
	{
		"512",     
		BeCast( 512 ),     
		"512"
	},
	{
		"1024",     
		BeCast( 1024 ),     
		"1024"
	},
	{ 0 }
};

static Be::VarChooser ShadowFilterChooser[] =
{
	{
		"None",     
		BeCast( Tr2InteriorBoxLight::ST_NONE ),     
		"No shadow"
	},
	{
		"StaticsOnly",     
		BeCast( Tr2InteriorBoxLight::ST_STATICS_ONLY ),     
		"Generate shadows from statics only"
	},
	{
		"DynamicOnly",     
		BeCast( Tr2InteriorBoxLight::ST_DYNAMICS_ONLY ),     
		"Generate shadows from statics only"
	},
	{
		"All",     
		BeCast( Tr2InteriorBoxLight::ST_ALL ),     
		"Generate shadows from statics and dynamics"
	},
	{ 0 }
};

const Be::ClassInfo* Tr2InteriorBoxLight::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorBoxLight, "" )
        MAP_INTERFACE( Tr2InteriorBoxLight )
        MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2InteriorLight )

		MAP_ATTRIBUTE( "name", m_name, "The name of this interior light source", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_transform, "Local to world transform matrix", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "The light's color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "kelvinColor", m_kelvinColor, "Kelvin color temperature", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useKelvinColor", m_useKelvinColor, "Use Kelvin color or RGB?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "Exponential falloff for distance from emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "specularIntensity", m_specularIntensity, "Additional specular light intensity multiplier", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shadowImportance", m_shadowImportance, "How important is this lightsource for shadowcasting", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "primaryLighting", m_primaryLighting, "Does this lightsource contribute to primary lighting?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "affectTransparentObjects", m_affectTransparentObjects, "Does this lightsource affect transparent objects?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "shadowResolution", m_shadowResolution, "Resolution of the shadow map for spot light", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, ShadowResolutionChooser )
		MAP_ATTRIBUTE( "enableShadowLOD", m_enableShadowLOD, "Enable dynamic shadow resolution based on light source screen size", Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER( "shadowCasterTypes", m_shadowCasterTypes, "Types of objects that cast shadow from this light source", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, ShadowFilterChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectedTexturePath", m_projectedTexturePath, "The path used to load the projected texture map", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriTextureChooser )
		MAP_ATTRIBUTE( "projectedTextureRes", m_projectedTextureRes, "Projected texture map", Be::READ )

		// Translation, rotation, scaling
		MAP_PROPERTY( "position", GetPosition, SetPosition, "Position of the placeable" )
		MAP_PROPERTY( "rotation", GetRotation, SetRotation, "Rotation of the placeable" )
		MAP_PROPERTY( "scaling", GetScaling, SetScaling, "Scale of the placeable" )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace", GetBoundingBoxInLocalSpace, "Gets the bounding box in local space" )
	EXPOSURE_END()
}

