// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerControllerTrigger.h"

BLUE_DEFINE( EveDistributionSpawnerControllerTrigger );

const Be::ClassInfo* EveDistributionSpawnerControllerTrigger::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnerControllerTrigger, ":jessica-icon: computer-classic\n" )
		MAP_INTERFACE( EveDistributionSpawnerControllerTrigger )
		MAP_INTERFACE( IEveDistributionSpawner )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "variableName", m_variableName, "activate and deactivate bucket based on this variable", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "value", m_value, "editable for authoring but triggered automatically from controller states", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "invertTrigger", m_invertReceivedValue, "editable for authoring but triggered automatically from controller states", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "restartOnReceivingValue", m_restartOnReceivingValue, "should it retrigger even if value is unchanged?", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "isActive", m_isActive, "for debug", Be::READ );

		MAP_ATTRIBUTE( "spawners", m_distributionSpawners, "list of spawners to activate based on trigger", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
