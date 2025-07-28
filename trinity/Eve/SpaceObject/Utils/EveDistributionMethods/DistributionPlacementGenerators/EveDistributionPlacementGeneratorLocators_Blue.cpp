#include "StdAfx.h"
#include "EveDistributionPlacementGeneratorLocators.h"

BLUE_DEFINE( EveDistributionPlacementGeneratorLocators );

const Be::ClassInfo* EveDistributionPlacementGeneratorLocators::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionPlacementGeneratorLocators, "" )
		MAP_INTERFACE( EveDistributionPlacementGeneratorLocators )
		MAP_INTERFACE( IEveDistributionPlacementGenerators )

		MAP_ATTRIBUTE( "locators", m_locators, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
