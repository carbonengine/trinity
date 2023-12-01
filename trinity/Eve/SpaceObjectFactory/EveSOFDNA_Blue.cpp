////////////////////////////////////////////////////////////
//
//    Created:   August 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "EveSOFDNA.h"

BLUE_DEFINE( EveSOFDNA );

const Be::ClassInfo* EveSOFDNA::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDNA, "" )
        MAP_INTERFACE( EveSOFDNA )

    EXPOSURE_END()
}
