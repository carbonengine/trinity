// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionPlacementGeneratorVolume.h"

BLUE_DEFINE( EveDistributionPlacementGeneratorVolume );

BLUE_DEFINE_INTERFACE( IEveDistributionPlacementGenerators );

const Be::ClassInfo* EveDistributionPlacementGeneratorVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionPlacementGeneratorVolume, ":jessica-icon: chart-scatter-3d\n" )
		MAP_INTERFACE( EveDistributionPlacementGeneratorVolume )
		MAP_INTERFACE( IEveDistributionPlacementGenerators )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "volume", m_volume, "Volume To generate points in", Be::PERSISTONLY )
		MAP_PROPERTY( "volume", GetVolume, SetVolume, "Volume To generate points in" )

		MAP_ATTRIBUTE( "numGenerated", m_numGenerated, "How many points to generate within the volume", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "hollowVolume", m_hollowVolume, "if enabled, the innerVolume will be carved out of the outer volume", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "falloffFactor", m_falloffFactor, "if enabled, the innerVolume will be carved out of the outer volume", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
