////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFDataMgr.h"
#include "EveSOFData.h"

#include "TriPythonContext.h"

BLUE_DEFINE( EveSOFDataMgr );

const Be::ClassInfo* EveSOFDataMgr::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataMgr, "" )
        MAP_INTERFACE( EveSOFDataMgr )

		MAP_METHOD_AND_WRAP( 
			"LoadData", 
			LoadData, 
			"Inject all the data into this mgr, providing a redfile path\n"
			":param filePath: res path to the data file"
			)
		MAP_METHOD_AND_WRAP( 
			"SetData", 
			SetData, 
			"Inject all the data into this mgr, providing a blue object\n"
			":param dbData: SOF data"
			)

		MAP_METHOD_AND_WRAP( 
			"UpdateHull", 
			UpdateHull, 
			"Update a specific hull\n"
			":param name: hull name\n"
			":param data: hull definition"
			)
		MAP_METHOD_AND_WRAP( 
			"UpdateFaction", 
			UpdateFaction, 
			"Update a specific faction\n"
			":param name: faction name\n"
			":param data: faction definition"
			)
		MAP_METHOD_AND_WRAP( 
			"UpdateRace", 
			UpdateRace, 
			"Update a specific race\n"
			":param name: race name\n"
			":param data: race definition"
			)
		MAP_METHOD_AND_WRAP( 
			"UpdateGeneric", 
			UpdateGeneric, 
			"Update the generic data\n"
			":param data: generic definition"
			)
		MAP_METHOD_AND_WRAP( 
			"UpdateMaterial", 
			UpdateMaterial, 
			"Update a specific material\n"
			":param name: material name\n"
			":param data: material definition"
			)
		MAP_METHOD_AND_WRAP( 
			"UpdatePattern", 
			UpdatePattern, 
			"Update a specific pattern\n" 
			":param name: pattern name\n"
			":param data: pattern definition"
			)
		MAP_METHOD_AND_WRAP(
			"UpdateLayout",
			UpdateLayout,
			"Update a specific layout\n"
			":param name: layout name\n"
			":param data: layout definition"
			)

		MAP_METHOD_AND_WRAP( 
			"HasFactionData", 
			HasFactionData, 
			"Does this faction exist?\n"
			":param name: faction name\n"
			)
		MAP_METHOD_AND_WRAP( 
			"HasHullData", 
			HasHullData, 
			"Does this hull exist?\n" 
			":param name: hull name\n"
			)
		MAP_METHOD_AND_WRAP( 
			"HasRaceData", 
			HasRaceData, 
			"Does this race exist?\n" 
			":param name: race name\n"
			)
		MAP_METHOD_AND_WRAP( 
			"HasMaterialData", 
			HasMaterialData, 
			"Does this material exist?\n" 
			":param name: material name\n"
			)
		MAP_METHOD_AND_WRAP( 
			"HasPatternData", 
			HasPatternData, 
			"Does this pattern exist?\n" 
			":param name: pattern name\n"
			)

    EXPOSURE_END()
}
