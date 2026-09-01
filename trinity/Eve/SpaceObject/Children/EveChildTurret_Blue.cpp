// Copyright © 2026 Fenris Creations ehf.

#include "StdAfx.h"
#include "EveChildTurret.h"
#include "Eve/Turret/EveTurretFiringFX.h"

BLUE_DEFINE( EveChildTurret );

extern Be::VarChooser ImpactBehaviourChooser[];

const Be::ClassInfo* EveChildTurret::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildTurret, "" )
		MAP_INTERFACE( EveChildTurret )


		MAP_ATTRIBUTE( "isOnline", m_isOnline, "Indicate if turret is active", Be::READWRITE )
		MAP_ATTRIBUTE( "trackingInfluence", m_trackingInfluence, "How much tracking is allowed?", Be::READ )
		MAP_ATTRIBUTE( "maxTrackingTime", m_maxTrackingTime, "How long does tracking take?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "state", m_state, "State of the turret", Be::READ | Be::PERSIST )

		MAP_PROPERTY( "targetObject", GetTargetObject, SetTargetObject, "object this turret will track" )
		MAP_ATTRIBUTE( "target", m_target, "Info on the target", Be::READ )

		MAP_ATTRIBUTE( "sysBoneHeight", m_aiming.m_sysBoneHeight, "System bone HEIGHT extension factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchFactor", m_aiming.m_sysBonePitchFactor, "main pitch factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchOffset", m_aiming.m_sysBonePitchOffset, "main pitch offset (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchMin", m_aiming.m_sysBonePitchMin, "main pitch minimum clamp value, prevents the turret from targeting down too much (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchMax", m_aiming.m_sysBonePitchMax, "main pitch maximum clamp value, prevents the turret from targeting down too much (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch01Factor", m_aiming.m_sysBonePitch01Factor, "pitch 01 factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch01Offset", m_aiming.m_sysBonePitch01Offset, "pitch 01 offset (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch02Factor", m_aiming.m_sysBonePitch02Factor, "pitch 02 factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch02Offset", m_aiming.m_sysBonePitch02Offset, "pitch 02 offset (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch03Factor", m_aiming.m_sysBonePitch03Factor, "pitch 03 factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch03Offset", m_aiming.m_sysBonePitch03Offset, "pitch 03 offset (in degrees!)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "maxCyclingFirePos", m_maxCyclingFirePos, "If greater than one we cycle through the given number of muzzles.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cyclingFireGroupCount", m_cyclingFireGroupCount, "The number of muzzles in one cycle group, usually only one.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentCyclingFiresPos", m_currentCyclingFiresPos, "Current muzzle id due to cycling muzzles", Be::READ )


		MAP_ATTRIBUTE( "firingEffect", m_firingEffect, "", Be::HIDDEN ) // Needed to make Graphite able to detect bindings inside of the firing effect.
		MAP_PROPERTY( "firingEffect", GetFiringEffect, SetFiringEffect, "The module for the firing effect of this turret" )
		MAP_ATTRIBUTE( "firingEffectResPath", m_firingEffectResPath, "A res path to the redfile containing the primary firing effect", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "impactSize", m_impactSize, "Size of impacts. No impact if size is 0 or less", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "impactBehaviour", m_impactBehaviour, "What do we want to hit? ", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, ImpactBehaviourChooser )

		MAP_ATTRIBUTE( "turretMovementObserver", m_turretMovementObserver, "The observer for turret movement sounds. Note: the positioning of this observer is automatic.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "playMovementSound", m_playMovementSound, "If true this turret set will play its mechanical movement sounds if movement audio events are defined.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "idleToTargetingMovementAudioEvent", m_idleToTargetingMovementAudioEvent, "The event to send to the audio engine for mechanical noise when a turret moves from idle to targeting.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "targetingToIdleMovementAudioEvent", m_targetingToIdleMovementAudioEvent, "The event to send to the audio engine for mechanical noise when a turret moves from targeting to idle.", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"EnterStateDeactive",
			EnterStateDeactive,
			"Go into state deactive: play deactive anim and stay inside ship. \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-bed\n" )

		MAP_METHOD_AND_WRAP(
			"EnterStateIdle",
			EnterStateIdle,
			"Go into state idle: play idle anim and face cannons forward. \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-male\n" )

		MAP_METHOD_AND_WRAP(
			"EnterStateTargeting",
			EnterStateTargeting,
			"Go into state targeting: face cannons towards enemy. \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-crosshairs\n" )

		MAP_METHOD_AND_WRAP(
			"EnterStateFiring",
			EnterStateFiring,
			"Go into state fire: play fire anim and face cannons towards enemy.\n:jessica-placement: TOOLBAR\n:jessica-icon: fa-fire-alt\n" )

		MAP_METHOD_AND_WRAP(
			"EnterStateReloading",
			EnterStateReloading,
			"Go into state reloading: play reload anim and then idle. \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-sync\n" )

		MAP_METHOD_AND_WRAP(
			"ForceStateDeactive",
			ForceStateDeactive,
			"Force into state deactive: no anim, no transition, just flip." )

		MAP_METHOD_AND_WRAP(
			"ForceStateTargeting",
			ForceStateTargeting,
			"Force into state targeting: no anim, no transition, just flip." )

		MAP_METHOD_AND_WRAP(
			"GetFiringBoneWorldTransform",
			GetFiringBoneWorldTransform,
			"Returns the world transform matrix of the specified firing bone in the currently firing turret."
			"\n:param idx: index of the firing bone in the current model."
			"\n:returns: The world transform matrix." )

	EXPOSURE_CHAINTO( EveChildMesh )
}
