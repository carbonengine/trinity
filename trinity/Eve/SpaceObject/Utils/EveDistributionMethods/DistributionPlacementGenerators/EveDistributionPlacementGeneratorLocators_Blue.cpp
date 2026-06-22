// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionPlacementGeneratorLocators.h"

BLUE_DEFINE( EveDistributionPlacementGeneratorLocators );

const Be::ClassInfo* EveDistributionPlacementGeneratorLocators::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionPlacementGeneratorLocators, ":jessica-icon: folder-tree\n" )
		MAP_INTERFACE( EveDistributionPlacementGeneratorLocators )
		MAP_INTERFACE( IEveDistributionPlacementGenerators )

		MAP_ATTRIBUTE( "locators", m_locators, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
