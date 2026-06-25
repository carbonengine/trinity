// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodRandom.h"

BLUE_DEFINE_INTERFACE( IEveProceduralSelectionMethod );

BLUE_DEFINE( EveProceduralMethodRandom );

const Be::ClassInfo* EveProceduralMethodRandom::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodRandom, "" )
		MAP_INTERFACE( EveProceduralMethodRandom )
		MAP_INTERFACE( IEveProceduralSelectionMethod )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "a descriptive name", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "seedName", m_seedName, "name of the gameplay attribute you want to use as a seed, not persisted /n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "seed_temp", m_seed, "temp until we hook in gameplay attributes, will be read only at that point", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "selectedChild", m_selectedChildIndex, "selection result", Be::READ )
		MAP_ATTRIBUTE( "totalWeight", m_totalWeight, "combined weighting of all the parameters", Be::READ )

		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "debugVolumes", m_debugVolumes, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
