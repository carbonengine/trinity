#include "StdAfx.h"

#include "Tr2InteriorLightSource.h"
#include "TriConstants.h"
#include "Resources/TriTextureRes.h"

BLUE_DEFINE( Tr2InteriorLightSource );

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
		BeCast( Tr2InteriorLightSource::ST_NONE ),     
		"No shadow"
	},
	{
		"StaticsOnly",     
		BeCast( Tr2InteriorLightSource::ST_STATICS_ONLY ),     
		"Generate shadows from statics only"
	},
	{
		"DynamicOnly",     
		BeCast( Tr2InteriorLightSource::ST_DYNAMICS_ONLY ),     
		"Generate shadows from statics only"
	},
	{
		"All",     
		BeCast( Tr2InteriorLightSource::ST_ALL ),     
		"Generate shadows from statics and dynamics"
	},
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( 
    "Tr2InteriorShadowCasterTypes", 
	ITr2InteriorLight::ShadowCasterTypes, 
    ShadowFilterChooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::ClassInfo* Tr2InteriorLightSource::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorLightSource, "" )
        MAP_INTERFACE( Tr2InteriorLightSource )
        MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2InteriorLight )

		MAP_ATTRIBUTE( "name", m_name, "The name of this interior light source", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "Position of light emitter", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "radius", m_radius, "Maximum distance from emitter", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "color", m_color, "The light's color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "kelvinColor", m_kelvinColor, "Kelvin color temperature", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useKelvinColor", m_useKelvinColor, "Use Kelvin color or RGB?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "Exponential falloff for distance from emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "specularIntensity", m_specularIntensity, "Additional specular light intensity multiplier", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shadowImportance", m_shadowImportance, "How important is this lightsource for shadowcasting", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "importanceScale", m_importanceScale, "Scale factor to multiply the light importance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "importanceBias",  m_importanceBias,  "Bias factor to add to the light importance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneAlphaOuter", m_coneAlphaOuter, "A spotlight's outer cone angle (Everything higher than 90 degrees results in a pointlight!)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "coneAlphaInner", m_coneAlphaInner, "A spotlight's inner cone angle", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "coneDirection", m_coneDirection, "A spotlight's direction", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "primaryLighting", m_primaryLighting, "Does this lightsource contribute to primary lighting?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "affectTransparentObjects", m_affectTransparentObjects, "Does this lightsource affect transparent objects?", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "shadowResolution", m_shadowResolution, "Resolution of the shadow map for spot light", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, ShadowResolutionChooser )
		MAP_ATTRIBUTE( "enableShadowLOD", m_enableShadowLOD, "Enable dynamic shadow resolution based on light source screen size", Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER( "shadowCasterTypes", m_shadowCasterTypes, "Types of objects that cast shadow from this light source", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, ShadowFilterChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectedTexturePath", m_projectedTexturePath, "The path used to load the projected texture map", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriTextureChooser )
		MAP_ATTRIBUTE( "projectedTextureRes", m_projectedTextureRes, "Projected texture map", Be::READ )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "IsSpotLight", IsSpotLight, "Returns true if the light is a spot light (cone angle < 90 degrees) ")
		MAP_METHOD_AND_WRAP( 
			"MarkShadowDirty", 
			MarkShadowDirty, 
			"Mark spotlight shadow as dirty to force its update\n"
			":param idx: shadow map index\n"
			":param dirty: dirty flag"
			)

		MAP_ATTRIBUTE( "customMaterial", m_customMaterial, "Custom shader material for light source", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_METHOD_AND_WRAP( "UpdateInternalMaterials", UpdateInternalMaterials, "Updates internal materials after customMaterial is changed")
	EXPOSURE_END()
}

