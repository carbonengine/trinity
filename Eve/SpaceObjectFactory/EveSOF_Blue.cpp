////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOF.h"

#include "Eve/Turret/EveTurretSet.h"

BLUE_DEFINE( EveSOF );

const Be::ClassInfo* EveSOF::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOF, "" )
        MAP_INTERFACE( EveSOF )

		MAP_ATTRIBUTE( "dataMgr", m_dataMgr, "Holds all the source data to the ships", Be::READ )

		MAP_METHOD_AND_WRAP( 
			"Build", 
			Build, 
			"Builds a space object\n"
			":param hullName: hull name\n"
			":param factionName: faction name\n"
			":param raceName: race name\n"
			)
		MAP_METHOD_AND_WRAP( 
			"BuildFromDNA", 
			BuildFromDNA, 
			"Builds a space object from the given DNA\n"
			":param dna: space object DNA"
			)
		MAP_METHOD_AND_WRAP( 
			"ValidateDNA", 
			ValidateDNA, 
			"Validates the content of a given DNA string. This is slow and should only be used for offline validation!\n"
			":param dna: space object DNA"
			)

		MAP_METHOD_AND_WRAP( 
			"SetupTurretMaterialFromDNA", 
			SetupTurretMaterialFromDNA, 
			"Change the material of the turret with SOF data\n"
			":param turretSet: turret set that will be modified\n"
			":param dna: DNA of the owner space object"
			)
		MAP_METHOD_AND_WRAP( 
			"SetupTurretMaterialFromFaction", 
			SetupTurretMaterialFromFaction, 
			"Change the material of the turret according to the faction in SOF\n"
			":param turretSet: turret set that will be modified\n"
			":param faction: faction name"
			)

    EXPOSURE_END()
}
