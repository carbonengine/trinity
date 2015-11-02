////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveTurretSet.h"
#include "EveTurretFiringFX.h"

#include "TriConstants.h"

BLUE_DEFINE( EveTurretSet );

Be::VarChooser LODChooser[] =
{
	{
		"LOD_INVALID",
			BeCast( EveTurretSet::LOD_INVALID ),
			""
	},
	{
		"LOD_EMPTY",
			BeCast( EveTurretSet::LOD_EMPTY ),
			""
	},
	{
		"LOD_HIGHEST",
			BeCast( EveTurretSet::LOD_HIGHEST ),
			""
	},
	{
		"LOD_DISABLED",
			BeCast( EveTurretSet::LOD_DISABLED ),
			""
	},
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( 
	"EveTurretSetLOD", 
	EveTurretSet::LOD, 
	LODChooser, 
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* EveTurretSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTurretSet, "" )
        MAP_INTERFACE( EveTurretSet )
        MAP_INTERFACE( IInitialize )
        MAP_INTERFACE( INotify )
        MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "name", m_name, "A name for this turret pair", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle rendering", Be::READWRITE )
		MAP_ATTRIBUTE( "displayEffects", m_displayEffects, "Toggle rendering of the attached firing effects", Be::READWRITE )
		MAP_ATTRIBUTE( "isOnline", m_isOnline, "Indicate if turret is active", Be::READWRITE )
		MAP_ATTRIBUTE( "visibleCount", m_visibleCount, "How many turrets are visible in this frame", Be::READ )
		MAP_ATTRIBUTE( "estimatedPixelDiameter", m_estimatedPixelDiameter, "value for LOD selection", Be::READ )
		MAP_ATTRIBUTE( "lodLevel", m_lodLevel, "current LOD", Be::READ )
		MAP_ATTRIBUTE( "trackingInfluence", m_trackingInfluence, "How much tracking is alowed?", Be::READ )

		MAP_ATTRIBUTE( "boundingSphere", m_boundingSphere, "Bounding sphere for visibility detection", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bottomClipHeight", m_bottomClipHeight, "Everything gets cut-off below this height (y-coord)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "locatorName", m_locatorName, "locator name for all turrets of this pair (A, B, C is auto-attached!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "slotNumber", m_slotNumber, "the slot number of the turret", Be::READWRITE )
		
		MAP_PROPERTY( "targetObject", GetTargetObject, SetTargetObject, "object this set of turrets will track"	)
//		MAP_ATTRIBUTE( "target", m_target, "Info on the target", Be::READ )

		MAP_ATTRIBUTE( "turretEffect", m_turretEffect, "The effect to use to draw the turret pair", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "geometryResource", m_geometryResource, "geometry resource for this turret, is read-only", Be::READ )
		MAP_ATTRIBUTE( "geometryResPath", m_geomResPath, "resource path to the turrets granny file", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE( "useRandomFiringDelay", m_useRandomFiringDelay, "Each firing could be hold of by a random amount of time", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomFiringDelay", m_randomFiringDelay, "The actual delay in seconds for this firing cycle", Be::READ )
		MAP_ATTRIBUTE( "maxCyclingFirePos", m_maxCyclingFirePos, "If greater than one we cycle through the given number of muzzles.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cyclingFireGroupCount", m_cyclingFireGroupCount, "The number of muzzles in one cycle group, usually only one.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentCyclingFiresPos", m_currentCyclingFiresPos, "Current muzzle id due to cycling muzzles", Be::READ )

		MAP_ATTRIBUTE( "sysBoneHeight", m_sysBoneHeight, "System bone HEIGHT extension factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchFactor", m_sysBonePitchFactor, "main pitch factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchOffset", m_sysBonePitchOffset, "main pitch offset (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitchMin", m_sysBonePitchMin, "main pitch minimum clamp value, prevents the turret from targeting down too much (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch01Factor", m_sysBonePitch01Factor, "pitch 01 factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch01Offset", m_sysBonePitch01Offset, "pitch 01 offset (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch02Factor", m_sysBonePitch02Factor, "pitch 02 factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch02Offset", m_sysBonePitch02Offset, "pitch 02 offset (in degrees!)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch03Factor", m_sysBonePitch03Factor, "pitch 03 factor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sysBonePitch03Offset", m_sysBonePitch03Offset, "pitch 03 offset (in degrees!)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "firingEffect", m_firingEffect, "The module for the firing effect of this turret", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "firingEffectResPath", m_firingEffectResPath, "A res path to the redfile containing the primary firing effect", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "laserMissBehaviour", m_laserMissBehaviour, "Whether or not to use laser-like properties when this turret misses", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "projectileMissBehaviour", m_projectileMissBehaviour, "Whether or not to use projectile properties when this turret misses", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"RebuildBoundingSphere",
			RebuildBoundingSphere,
			"Force a recalculation of the internal bounding sphere data of this turret set." )

		MAP_METHOD_AND_WRAP(
			"FreezeHighDetailLOD",
			FreezeHighDetailLOD,
			"Freezes the high detail LOD and prevents LOD selection or resource unloading." )

		MAP_METHOD_AND_WRAP(
			"EnterStateDeactive",
			EnterStateDeactive,
			"Go into state deactive: play deactive anim and stay inside ship." )

		MAP_METHOD_AND_WRAP(
			"EnterStateIdle",
			EnterStateIdle,
			"Go into state idle: play idle anim and face cannons forward." )

		MAP_METHOD_AND_WRAP(
			"EnterStateTargeting",
			EnterStateTargeting,
			"Go into state targeting: face cannons towards enemy." )

		MAP_METHOD_AND_WRAP(
			"EnterStateFiring",
			EnterStateFiring,
			"Go into state fire: play fire anim and face cannons towards enemy." )

		MAP_METHOD_AND_WRAP(
			"EnterStateReloading",
			EnterStateReloading,
			"Go into state reloading: play reload anim and then idle." )

		MAP_METHOD_AND_WRAP(
			"ForceStateDeactive",
			ForceStateDeactive,
			"Force into state deactive: no anim, no transition, just flip." )

		MAP_METHOD_AND_WRAP(
			"SetShotMissed",
			SetShotMissed,
			"Set whether the last turret shot missed." )
		
		MAP_METHOD_AND_WRAP(
			"GetLastShotTime",
			GetLastShotTime,
			"Get the time we last fired in arbitrary units. Only use for comparison between turret sets." )

		MAP_METHOD_AND_WRAP(
			"GetShotTimeVariance",
			GetShotTimeVariance,
			"Get maximum time variance between turrets in a set.")

		MAP_METHOD_AND_WRAP(
			"MissQueueSize",
			MissQueueSize,
			"Get the size of the active miss/hit queue.")

		MAP_METHOD_AND_WRAP(
			"GetFiringBoneWorldTransform",
			GetFiringBoneWorldTransform,
			"Returns the world transform matrix of the specfified firing bone in the currently firing turret."
			"\n"
			"\nArguments:"
			"\ni - The number of the firing bone in the current model."
			"\nReturn value:"
			"\nThe world transform matrix.")

	EXPOSURE_END()
}
