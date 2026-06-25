// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "EveChildExplosion.h"


BLUE_DEFINE( EveChildExplosion );

const Be::ClassInfo* EveChildExplosion::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildExplosion, "Specialized explosion space object child" )
		MAP_INTERFACE( EveChildExplosion )
		MAP_INTERFACE( EveChildContainer )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectChild )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"localExplosion",
			m_localExplosion,
			"Child containing local explosion effect",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"localExplosions",
			m_localExplosions,
			"List of children containing versions of local explosion effect",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"localExplosionShared",
			m_localExplosionShared,
			"Child containing shared parts of the local explosion effect (particle systems etc.)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"globalExplosion",
			m_globalExplosion,
			"Child containing global explosion effect",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"globalExplosions",
			m_globalExplosions,
			"Children containing global explosion effects",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"localExplosionDelay",
			m_localExplosionDelay,
			"Delay from explosion start to the first \"local\" explosion in seconds",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"localExplosionInterval",
			m_localExplosionInterval,
			"Maximum interval between local explosions in seconds",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"localExplosionIntervalFactor",
			m_localExplosionIntervalFactor,
			"Coefficent to apply to m_localExplosionInterval for consecutive explosions",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"globalExplosionDelay",
			m_globalExplosionDelay,
			"Delay from the end of the last \"local\" explosion to the start of the \"global\" explosion in seconds",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"wreckSwitchTime",
			m_wreckSwitchTime,
			"Time from the start of the explosion to the point when original model needs to be switched to the wreck",
			Be::READ )
		MAP_ATTRIBUTE(
			"wreckSwitchOffsetFromGlobalStart",
			m_wreckSwitchOffsetFromGlobalStart,
			"Time from the start of the global explosion that the model will be switched",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"localDuration",
			m_localDuration,
			"Total duration of a single local explosion",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"globalDuration",
			m_globalDuration,
			"Duration of the global explosion",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"totalDuration",
			m_totalDuration,
			"Duration of the complete explosion",
			Be::READ )

		MAP_ATTRIBUTE(
			"globalExplosionTime",
			m_globalExplosionTime,
			"The start of the global explosion",
			Be::READ )

		MAP_ATTRIBUTE(
			"isPlaying",
			m_isPlaying,
			"Is the effect playing",
			Be::READ )
		MAP_ATTRIBUTE(
			"playTime",
			m_playTime,
			"Time since the start of playback",
			Be::READ )
		MAP_ATTRIBUTE( "generatedLocalExplosions", m_objects, "", Be::READ )
		MAP_ATTRIBUTE( "generatedGlobalExplosions", m_globalExplosionContainer, "", Be::READ )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localScaling", m_localExplosionScaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "globalScaling", m_globalExplosionScaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )

		MAP_METHOD_AND_WRAP(
			"Play",
			Play,
			"Starts effect playback.\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png" )
		MAP_METHOD_AND_WRAP(
			"Stop",
			Stop,
			"Stops effect playback.\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png" )
		MAP_METHOD_AND_WRAP(
			"SetLocalExplosionTransforms",
			SetLocalExplosionTransforms,
			"Assigns transforms for local explosions.\n"
			":param transforms: list of matricies to be used as local explosion transforms" )
		MAP_METHOD_AND_WRAP(
			"SetGlobalExplosionOffset",
			SetGlobalExplosionOffset,
			"Sets the global explosion offset.\n"
			":param offset: Vector3 that to be used as the global explosion offset should be in local coordinates" )

	EXPOSURE_END()
}
