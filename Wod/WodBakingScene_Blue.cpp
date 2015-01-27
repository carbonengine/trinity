#include "StdAfx.h"

#include "WodBakingScene.h"

BLUE_DEFINE( WodBakingScene );

static Be::VarChooser VisualizerChooser[] =
{
	{
		"None",     
		BeCast( VM_NONE ),     
		"No visualizer - use normal rendering"
	},
	{
		"White",     
		BeCast( VM_WHITE ),     
		"Pixel shader returns white (useful to verify that something is output)"
	},

	{
		"Object normal",     
		BeCast( VM_OBJECT_NORMAL ),     
		"Normal from vertices"
	},
	{
		"Tangent",     
		BeCast( VM_TANGENT ),     
		"Tangent from vertices"
	},
	{
		"Bitangent",     
		BeCast( VM_BITANGENT ),     
		"Bitangent from vertices"
	},

	{
		"TexCoord0",     
		BeCast( VM_TEXCOORD0 ),     
		"Texture coordinate 0"
	},
	{
		"TexCoord1",     
		BeCast( VM_TEXCOORD1 ),     
		"Texture coordinate 0"
	},

	{
		"TexelDensity0",     
		BeCast( VM_TEXELDENSITY0 ),     
		"Density of texels mapped through texture coordinate 0"
	},
	{
		"Normal map",     
		BeCast( VM_NORMALMAP ),     
		"Tangent space normal from map"
	},
	{
		"Diffuse map",     
		BeCast( VM_DIFFUSEMAP ),     
		"Diffuse map"
	},
	{
		"Specular map",     
		BeCast( VM_SPECULARMAP ),     
		"Specular map"
	},
	{
		"Overdraw",
		BeCast( VM_OVERDRAW ),
		"See the overdraw of the scene"
	},
	{
		"Enlighten only",
		BeCast( VM_EN_ONLY ),
		"Enlighten only)"
	},
	{
		"Depth",
		BeCast( VM_DEPTH ),
		"See the depth buffer of the scene"
	},
	{ 0 }
};


const Be::ClassInfo* WodBakingScene::ExposeToBlue()
{
	EXPOSURE_BEGIN( WodBakingScene, "" )
		MAP_INTERFACE( WodBakingScene )
		MAP_INTERFACE( ITr2Scene )

		MAP_ATTRIBUTE_WITH_CHOOSER( "visualizeMethod", m_visualizeMethod, "Legacy attribute, do not use. TODO: remove me", Be::READWRITE | Be::ENUM | Be::NOTIFY, VisualizerChooser )

		MAP_ATTRIBUTE( 
			"Avatar", 
			m_skinnedObject, 
			"The avatar to be baked in this scene", Be::READWRITE )

	EXPOSURE_END()
}

