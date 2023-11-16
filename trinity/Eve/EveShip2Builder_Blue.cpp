#include "StdAfx.h"

#include "EveShip2Builder.h"
#include "SpaceObject/EveShip2.h"

BLUE_DEFINE( EveShip2Builder );

const Be::ClassInfo* EveShip2Builder::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveShip2Builder, "" )
        MAP_INTERFACE( EveShip2Builder )
		
		MAP_ATTRIBUTE( "grannyResources", m_grannyResources, "All the granny resources we want to combine", Be::READ )
		MAP_ATTRIBUTE( "hulls", m_hulls, "All the SOF hulls we want to combine", Be::READ )
		MAP_ATTRIBUTE( "outputFilename", m_outputFilename, "The path to the resulting geometry file", Be::READWRITE )
		MAP_METHOD_AND_WRAP("InitializeGrannyFile", InitializeGrannyFile, "Initializes Granny Geometry Combination")
		MAP_METHOD_AND_WRAP(
			"CombineGrannyGeometry", 
			CombineGrannyGeometry, 
			"Combines Granny Geometry.\n"
			":param idx: granny resource index\n"
			":param offset: element transform"
		)
		MAP_METHOD_AND_WRAP("CombineHullGeometry", CombineHullGeometry, "Combines Hull Geometry.")
		MAP_METHOD_AND_WRAP(
			"FinalizeGrannyFile", 
			FinalizeGrannyFile, 
			"Finalizes Granny Geometry Combination\n"
			":param path: output file path\n"
			":param combineGroups: combine groups (areas) into one"
		)
    EXPOSURE_END()
}
