#include "StdAfx.h"

#include "Tr2InteriorCell.h"
#include "Tr2InteriorStatic.h"

BLUE_DEFINE( Tr2InteriorCell );


// ------------------------------------------------------------------------------------------------------
const Be::ClassInfo* Tr2InteriorCell::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorCell, "" )
        MAP_INTERFACE( Tr2InteriorCell )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "statics", m_statics, "A list of Tr2InteriorStatics, which receive secondary lighting from cube maps", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reflectionMapPath", m_reflectionMapPath, "The path for the reflection map for this cell", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "reflectionMap", m_reflectionMapRes, "The reflection map for this cell", Be::READ )
		MAP_ATTRIBUTE( "irradianceTexture", m_irradianceTexture, "Path to irradiance light map", Be::READ );
		MAP_ATTRIBUTE( "directionalIrradianceTexture", m_directionalIrradianceTexture, "Irradiance light map", Be::READ );
		MAP_ATTRIBUTE( "irradianceTexturePath", m_irradianceTexturePath, "Path to directional irradiance light map", Be::READWRITE | Be::PERSIST | Be::NOTIFY );
		MAP_ATTRIBUTE( "directionalIrradianceTexturePath", m_directionalIrradianceTexturePath, "Directional irradiance light map", Be::READWRITE | Be::PERSIST | Be::NOTIFY );

		MAP_ATTRIBUTE( "minBounds", m_minBounds, "DEPRECATED: world (x,y,z) minimum bounds for the cell", Be::READ )
		MAP_ATTRIBUTE( "maxBounds", m_maxBounds, "DEPRECATED: world (x,y,z) maximum bounds for the cell", Be::READ )

		MAP_ATTRIBUTE( "isUnbounded", m_isUnbounded, "Set to true if the cell has no static geometry, but should still be able to contain dynamics and lights", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "drawBoundingBox", m_drawBoundingBox, "renderDebugInfo must be on!", Be::READWRITE )

		MAP_ATTRIBUTE( "shProbeResPath", m_shProbeResPath, "a resource path for cached spherical harmonic probe solutions", Be::READWRITE | Be::PERSIST| Be::NOTIFY )
		MAP_ATTRIBUTE( "shProbeResource", m_shProbeResource, "a resource for cached spherical harmonic probe solutions", Be::READ )

		MAP_ATTRIBUTE( "variableStore", m_variableStore, "Local variable store for this cell", Be::READ )

		MAP_METHOD_AND_WRAP( "RebuildInternalData", RebuildInternalData, "Notify this cell that its content has changed, so it can rebuild itself" )

		MAP_METHOD_AND_WRAP( 
			"AddStatic", 
			AddStatic, 
			"Add an interior static object to this cell\n"
			"\n:param interiorStatic: The Tr2InteriorStatic to add" );

		MAP_METHOD_AND_WRAP( 
			"RemoveStatic", 
			RemoveStatic, 
			"Remove a Tr2InteriorStatic from this cell"
			"\n:param interiorStatic: The Tr2InteriorStatic to remove");

		MAP_METHOD_AND_WRAP( "ClearStatics", ClearStatics, "Remove ALL interior static objects from this cell" );
	EXPOSURE_END()
}

