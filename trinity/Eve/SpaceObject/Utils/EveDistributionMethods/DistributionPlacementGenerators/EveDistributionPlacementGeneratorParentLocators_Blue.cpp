// Copyright © 2025 CCP ehf.

#include "EveDistributionPlacementGeneratorParentLocators.h"

BLUE_DEFINE( EveDistributionPlacementGeneratorParentLocators );

const Be::ClassInfo* EveDistributionPlacementGeneratorParentLocators::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionPlacementGeneratorParentLocators, ":jessica-icon: person-pregnant\n" )
		MAP_INTERFACE( EveDistributionPlacementGeneratorParentLocators )
		MAP_INTERFACE( IEveDistributionPlacementGenerators )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "name of the parent locator set", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
