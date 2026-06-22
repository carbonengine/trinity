// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveBaseDistributionMethod.h"


BLUE_DEFINE_INTERFACE( IEveDistributionMethod );
BLUE_DEFINE_INTERFACE( IEveDistributionRulesParent );

BLUE_DEFINE( EveBaseDistributionMethod );

const Be::ClassInfo* EveBaseDistributionMethod::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveBaseDistributionMethod, ":jessica-icon: map-location\n" )
		MAP_INTERFACE( EveBaseDistributionMethod )
		MAP_INTERFACE( IEveDistributionMethod )
		MAP_INTERFACE( IEveDistributionRulesParent )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "placementData", m_placementData, "modify for debugging only\n:jessica-hidden: True", Be::READ )

		MAP_ATTRIBUTE( "placementGenerators", m_placementGenerators, "initialPLacementGenerators. creates the initial dataSet on start", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "spawnTriggers", m_distributionSpawners, "trigger one of the generated initial transforms", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "spawnModifiers", m_distributionSpawnModifiers, "modify spawned entity", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lifetimeModifiers", m_distributionModifiers, "affect placementData on runtime", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "locationsCanReTrigger", m_locationsCanReTrigger, "after a location triggers it will rearm in a bit\n:jessica-group: BaseDistributionSettings", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "timeOutOnTriggering", m_timeOutOnTriggering, "duration until the same trigger can trigger again\n:jessica-group: BaseDistributionSettings", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "playtimeMultiplier", m_playtimeMultiplier, "if you want to speed up or slow down the system \n:jessica-group: BaseDistributionSettings", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "freePlacements", m_numFreePlacements, "for debugging purposes\n:jessica-group: BaseDistributionSettings", Be::READ );
		MAP_ATTRIBUTE( "entitiesSpawned", m_uniqueIDCounter, "for debugging purposes (total, not only during current activation)\n:jessica-group: BaseDistributionSettings", Be::READ );

		MAP_METHOD_AND_WRAP( "AddEntity", AddEntities, "add entity \n:param count: how many\n:jessica-placement: TOOLBAR" )
		MAP_METHOD_AND_WRAP( "Restart", RestartDistribution, "reset/restart \n:jessica-placement: TOOLBAR" )

	EXPOSURE_END()
}
