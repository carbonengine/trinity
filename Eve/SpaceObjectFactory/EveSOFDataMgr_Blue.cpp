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

		MAP_METHOD_AND_WRAP( "LoadData", LoadData, "Inject all the data into this mgr, providing a redfile path" )
		MAP_METHOD_AND_WRAP( "SetData", SetData, "Inject all the data into this mgr, providing a blue object" )

		MAP_METHOD_AND_WRAP( "UpdateHull", UpdateHull, "Update a specific hull" )
		MAP_METHOD_AND_WRAP( "UpdateFaction", UpdateFaction, "Update a specific faction" )
		MAP_METHOD_AND_WRAP( "UpdateRace", UpdateRace, "Update a specific race" )
		MAP_METHOD_AND_WRAP( "UpdateMaterial", UpdateMaterial, "Update a specific material" )

		MAP_METHOD_AND_WRAP( "HasFactionData", HasFactionData, "Does this faction exist?" )
		MAP_METHOD_AND_WRAP( "HasHullData", HasHullData, "Does this hull exist?" )
		MAP_METHOD_AND_WRAP( "HasRaceData", HasRaceData, "Does this race exist?" )

    EXPOSURE_END()
}
