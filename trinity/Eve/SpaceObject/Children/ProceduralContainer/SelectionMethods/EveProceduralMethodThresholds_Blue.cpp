////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#include "EveProceduralMethodThresholds.h"

BLUE_DEFINE( EveProceduralMethodThresholds );

const Be::ClassInfo* EveProceduralMethodThresholds::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveProceduralMethodThresholds, "" )
        MAP_INTERFACE( EveProceduralMethodThresholds )
        MAP_INTERFACE( IEveProceduralSelectionMethod )
        MAP_INTERFACE( INotify )
        MAP_INTERFACE( IListNotify )
        MAP_INTERFACE( IInitialize )

        MAP_ATTRIBUTE( "name", m_name, "a descriptive name", Be::READWRITE | Be::PERSIST )

        MAP_ATTRIBUTE( "thresholdAttribute", m_thresholdAttribute, "name of the gameplay attribute you want to use as a seed, not persisted /n", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "seed_temp", m_seed, "temp until we hook in gameplay attributes, will be read only at that point", Be::READWRITE | Be::NOTIFY )
        MAP_ATTRIBUTE( "selectedChild", m_selectedChildIndex, "selection result", Be::READ )

        MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
        MAP_ATTRIBUTE( "debugVolumes", m_debugVolumes, "", Be::READ | Be::PERSIST )

    EXPOSURE_END()
}


