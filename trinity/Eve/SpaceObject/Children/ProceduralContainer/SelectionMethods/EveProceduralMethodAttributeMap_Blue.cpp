// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodAttributeMap.h"

BLUE_DEFINE( EveProceduralMethodAttributeMap );

const Be::ClassInfo* EveProceduralMethodAttributeMap::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodAttributeMap, "" )
		MAP_INTERFACE( EveProceduralMethodAttributeMap )
		MAP_INTERFACE( IEveProceduralSelectionMethod )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "thresholdAttribute", m_mappedAttribute, "name of the gameplay attribute you want to use as a seed, not persisted /n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "seed_temp", m_seed, "temp until we hook in gameplay attributes, will be read only at that point", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "selectedChild", m_selectedChildIndex, "selection result", Be::READ )

		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "debugVolumes", m_debugVolumes, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
