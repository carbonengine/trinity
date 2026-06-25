// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveSwarm.h"


BLUE_DEFINE( EveSwarmRenderable );
const Be::ClassInfo* EveSwarmRenderable::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSwarmRenderable, "" )
		MAP_INTERFACE( ITr2Pickable )
			MAP_INTERFACE( EveEntity )
				MAP_INTERFACE( IEveShadowCaster )
					EXPOSURE_END()
}


BLUE_DEFINE( EveSwarm );
const Be::ClassInfo* EveSwarm::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSwarm, "" )
		MAP_INTERFACE( EveSwarm )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveShadowCaster )

		MAP_ATTRIBUTE( "count", m_count, ":jessica-group: Swarm", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "swarmingEnabled", m_swarmingEnabled, ":jessica-group: Swarm", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "mass", m_behavior.m_mass, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speedMultiplier", m_behavior.m_speedMultiplier, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speedMinimum", m_behavior.m_speedMinimum, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxDistance0", m_behavior.m_maxDistance0, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxDistance1", m_behavior.m_maxDistance1, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxTime", m_behavior.m_maxTime, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "timeMultiplier", m_behavior.m_timeMultiplier, ":jessica-group: Swarm", Be::READWRITE )
		MAP_ATTRIBUTE( "agility", m_behavior.m_agility, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speed0", m_behavior.m_speed0, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speed1", m_behavior.m_speed1, ":jessica-group: Swarm", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "weightFormation", m_behavior.m_weightFormation, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightCohesion", m_behavior.m_weightCohesion, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightSeparation", m_behavior.m_weightSeparation, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightAlign", m_behavior.m_weightAlign, "This should be in newtons, based of average direction of swarmers.\n:jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightWander", m_behavior.m_weightWander, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightAnchor", m_behavior.m_weightAnchor, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "anchorRadius0", m_behavior.m_anchorRadius0, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "anchorRadius1", m_behavior.m_anchorRadius1, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightDeceleration", m_behavior.m_weightDecelerate, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxDeceleration", m_behavior.m_maxDeceleration, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "separationDistance", m_behavior.m_separationDistance, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "formationDistance", m_behavior.m_formationDistance, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "wanderFluctuation", m_behavior.m_wanderFluctuation, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "wanderDistance", m_behavior.m_wanderDistance, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "wanderRadius", m_behavior.m_wanderRadius, ":jessica-group: Behavior", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "debugShowForces", m_debugShowForces, ":jessica-group: Debug", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "AddSwarmer", AddSwarmer, "" )
		MAP_METHOD_AND_WRAP( "RemoveSwarmer", RemoveSwarmer, "" )
		MAP_METHOD_AND_WRAP(
			"SetCount",
			SetCount,
			"Set number of things in the swarm\n"
			":param count: number of things in the swarm" )
		MAP_METHOD_AND_WRAP(
			"EnableSwarming",
			EnableSwarming,
			"Enable/disable swarming\n"
			":param enable: enable/disable swarming" )
		MAP_METHOD_AND_WRAP( "PickFiringOrigin", PickFiringOrigin, "" )

	EXPOSURE_CHAINTO( EveShip2 )
}