// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionModifierProcessLifetime.h"

const Be::VarChooser LifetimeEventChooser[] = {
	{ "Do nothing", BeCast( DistributionEntityLifeTimeEvent::DO_NOTHING ), "do nothing, use for debug only" },
	{ "Kill entity", BeCast( DistributionEntityLifeTimeEvent::KILL_ENTITY ), "destroy precessed entity when lifeTime > eventAtTime" },
	{ "Kill and Spawn new", BeCast( DistributionEntityLifeTimeEvent::KILL_AND_SPAWN_NEW_FROM_DISTRIBUTION ), "kill + creates a new random entity by parent Distribution" },
	{ "Kill and spawn from my initial placement", BeCast( DistributionEntityLifeTimeEvent::KILL_AND_SPAWN_NEW_FROM_INITIAL_POSITION ), "kill + creates a new entity from the same initial parameters" },
	{ "Kill and spawn new from current location", BeCast( DistributionEntityLifeTimeEvent::KILL_AND_SPAWN_NEW_FROM_CURRENT_POSITION ), "kill + creates a new entity from the current parameters" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "lifetimeEvent", DistributionEntityLifeTimeEvent, LifetimeEventChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE_INTERFACE( IEveDistributionModifier );

BLUE_DEFINE( EveDistributionModifierProcessLifetime );

const Be::ClassInfo* EveDistributionModifierProcessLifetime::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionModifierProcessLifetime, ":jessica-icon: person-cane\n" )
		MAP_INTERFACE( EveDistributionModifierProcessLifetime )
		MAP_INTERFACE( IEveDistributionModifier )

		MAP_ATTRIBUTE( "lifetimeDuration", m_lifetimeDuration, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE_WITH_CHOOSER( "killEvent", m_killEvent, "", Be::READWRITE | Be::PERSIST | Be::ENUM, LifetimeEventChooser );

	EXPOSURE_END()
}
