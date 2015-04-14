////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOF.h"

#include "Eve/EveTurretSet.h"

BLUE_DEFINE( EveSOF );

const Be::ClassInfo* EveSOF::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOF, "" )
        MAP_INTERFACE( EveSOF )

		MAP_ATTRIBUTE( "dataMgr", m_dataMgr, "Holds all the source data to the ships", Be::READ )

		MAP_METHOD_AND_WRAP( "Build", Build, "na" )
		MAP_METHOD_AND_WRAP( "BuildFromDNA", BuildFromDNA, "na" )
		MAP_METHOD_AND_WRAP( "ValidateDNA", ValidateDNA, "Validates the content of a given DNA string. This is slow and should only be used for offline validation!" )

		MAP_METHOD_AND_WRAP( "SetupTurretMaterial", SetupTurretMaterial, "na" )

    EXPOSURE_END()
}
