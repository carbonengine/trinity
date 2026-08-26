// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveImpactOverlay.h"

#include "Curves/Fader/Tr2ScalarFader.h"
#include "Shader/Tr2Effect.h"
#include "TriSequencer.h"

BLUE_DEFINE( EveImpactOverlay );

const Be::ClassInfo* EveImpactOverlay::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveImpactOverlay, "" )
		MAP_INTERFACE( EveImpactOverlay )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldImpactColorFade", m_shieldImpactColorFade, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldImpactParentSize", m_shieldImpactParentSize, "", Be::READ )
		MAP_ATTRIBUTE( "shieldIsEllipsoid", m_shieldIsEllipsoid, "", Be::READWRITE )

		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "maxShieldImpacts", m_maxShieldImpacts, "", Be::READ )
		MAP_ATTRIBUTE( "overallShieldImpact", m_overallShieldImpact, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldHardening", m_shieldHardening, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldBoosting", m_shieldBoosting, "", Be::READWRITE )

		MAP_ATTRIBUTE( "armorImpactEmitter", m_armorImpactEmitter, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullImpactEmitter", m_hullImpactEmitter, "The hull impact emitter", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "damageOverlay", m_damageOverlay, "The armor/hull damage overlay, shared data-texture block owner", Be::READ )

		// armor/hull state lives on the damage overlay, keep the old names working
		MAP_PROPERTY_READONLY( "seed", GetSeed, "" )
		MAP_PROPERTY_READONLY( "configuration", GetImpactConfiguration, "Impact goes into what?" )
		MAP_PROPERTY_READONLY( "impactDataNextIdx", GetImpactDataNextIdx, "" )
		MAP_PROPERTY_READONLY( "armorImpactGoalCount", GetArmorImpactGoalCount, "" )
		MAP_PROPERTY_READONLY( "armorImpactParentSize", GetArmorImpactParentSize, "" )
		MAP_PROPERTY( "debugForceSpawnDebris", GetDebugForceSpawnDebris, SetDebugForceSpawnDebris, "" )
		MAP_PROPERTY_READONLY( "renderPriority", GetRenderPriority, "" )
		MAP_PROPERTY_READONLY( "dataTextureBlockID", GetDataTextureBlockID, "The ID for our part in the big texture." )
		MAP_PROPERTY_PERSISTED( "armorDamageShader", GetArmorDamageShaderEffect, SetArmorDamageShaderEffect, "" )
		MAP_PROPERTY( "armorRepairing", GetArmorRepairing, SetArmorRepairing, "" )
		MAP_PROPERTY( "armorHardening", GetArmorHardening, SetArmorHardening, "" )
		MAP_PROPERTY( "hullRepairing", GetHullRepairing, SetHullRepairing, "" )
		MAP_PROPERTY_PERSISTED( "hullDamageFlickerCurve", GetHullDamageFlickerCurve, SetHullDamageFlickerCurve, "This is the flickering for hull damage" )
		MAP_PROPERTY( "hullDamageFactor", GetHullDamageFactor, SetHullDamageFactor, "How much hull damage to show?" )

		MAP_METHOD_AND_WRAP(
			"GetArmorImpactLifeTime",
			GetArmorImpactLifeTime,
			"Value for how long the overlay effect plays.\n" )
		MAP_METHOD_AND_WRAP(
			"GetLastDamageState",
			GetLastDamageState,
			"Last configured damage state (shield, armor, hull).\n" )

	EXPOSURE_END()
}
