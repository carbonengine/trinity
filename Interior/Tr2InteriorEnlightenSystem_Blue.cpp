#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorEnlightenSystem.h"
#include "Tr2InteriorStatic.h"


static Be::VarChooser EnlightenQualityTypeChooser[] =
{
	{
		"IrradianceOnly",     
		BeCast( Tr2InteriorEnlightenSystem::IRRADIANCE ),     
		"Use irradiance-only Enlighten textures"
	},
	{
		"DirectionalIrradiance",     
		BeCast( Tr2InteriorEnlightenSystem::DIRECTIONAL_IRRADIANCE ),     
		"Use directional irradiance Enlighten textures"
	},
	{
		"SphericalHarmonics",     
		BeCast( Tr2InteriorEnlightenSystem::SPHERICAL_HARMONICS ),     
		"Use spherical harmonics Enlighten textures"
	},
	{ 0 }
};

BLUE_DEFINE( Tr2InteriorEnlightenSystem );

const Be::ClassInfo* Tr2InteriorEnlightenSystem::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorEnlightenSystem, "" )
        MAP_INTERFACE( Tr2InteriorEnlightenSystem )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ID3DTexture )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "The name of the sub-cell", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minBounds", m_minBounds, "world (x,y,z) minimum bounds of the system", Be::READ )
		MAP_ATTRIBUTE( "maxBounds", m_maxBounds, "world (x,y,z) maximum bounds of the system", Be::READ )
		MAP_ATTRIBUTE( "drawBoundingBox", m_drawBoundingBox, "renderDebugInfo must be on!", Be::READWRITE )

		MAP_ATTRIBUTE( "updateRadiosity", m_updateRadiosity, "If the radiosity bounce for the system should be updated", Be::READWRITE )
		MAP_ATTRIBUTE( "updateInputLighting", m_updateInputLighting, "If the input lighting for the system should be updated", Be::READWRITE )
		MAP_ATTRIBUTE( "renderDebugDusterLighting", m_debugRenderDusterLighting, "If we should render the duster lighting to the debug renderer", Be::READWRITE )

		MAP_ATTRIBUTE_WITH_CHOOSER( 
			"enlightenQuality", 
			m_enlightenQuality, 
			"Type of Enlighten texturing for statics in this system", 
			Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, 
			EnlightenQualityTypeChooser )

		MAP_ATTRIBUTE( "statics", m_statics, "A list of Tr2InteriorStatics, which take part in the Enlighten radiosity solution", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "irradianceScale", m_irradianceScale, "The overall scale of the irradiance contribution within this system", Be::READWRITE| Be::PERSIST  )
		MAP_ATTRIBUTE( "bounceScale", m_bounceScale, "A factor for diminishing the amount of light from each bounce in enlighten", Be::READWRITE| Be::PERSIST  )
		MAP_ATTRIBUTE( "enlightenPixelSize", m_enlightenSystem.m_enlightenPixelSize, "The size of the pixels for the radiosity that enlighten tries to create, in meters", Be::READWRITE| Be::PERSIST  )
		MAP_ATTRIBUTE( "radSystemPath", m_enlightenSystem.m_radResPath, "A res path to a cached radiosity system for Enlighten (including dusters)", Be::READWRITE | Be::PERSIST| Be::NOTIFY )
		MAP_ATTRIBUTE( "systemID", m_enlightenSystem.m_systemID, "A unique ID for this system", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "systemInCellID", m_enlightenSystem.m_systemInCellIdx, "An ID based on the order of this system within the Tr2InteriorCell, it is used for material assignments.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "radiosityResource", m_enlightenSystem.m_radSystemResource, "The resource for the Enlighten Radiosity System", Be::READ )
		MAP_ATTRIBUTE( "pixelDimensions", m_pixelDimensions, "The pixel dimensions of the enlighten system", Be::READ )

		MAP_ATTRIBUTE( "variableStore", m_variableStore, "Local variable store for this system", Be::READ )
		MAP_ATTRIBUTE( "useTextureAlbedo", m_useTextureAlbedo, "Use automatically generated albedo textures for Enlighten", Be::READWRITE )

		MAP_METHOD_AND_WRAP( 
			"AddStatic", 
			AddStatic, 
			"Add an interior static object to this system"
			"\n"
			"\nArguments:"
			"\ninteriorStatic - The Tr2InteriorStatic to add" );

		MAP_METHOD_AND_WRAP( 
			"RemoveStatic", 
			RemoveStatic, 
			"Remove a Tr2InteriorStatic from this system"
			"\n"
			"\nArguments:"
			"\ninteriorStatic - The Tr2InteriorStatic to remove");

		MAP_METHOD_AND_WRAP( "ClearStatics", ClearStatics, "Remove ALL interior static objects from this system" );
		MAP_METHOD_AND_WRAP( 
			"SetSystemInCellIdx", 
			SetSystemInCellIdx, 
			"\nSet the system in cell idx.Should only be used for data that has been loaded from database and not disk.\n"
			"\n"
			"\nArguments:"
			"\nidx - The system cell index"
			);
		MAP_METHOD_AND_WRAP( "UpdateEnlightenMaterialTextures", UpdateEnlightenMaterialTextures, "Updates albedo and emissive Enlighten textures" );
	EXPOSURE_END()
}

#endif
