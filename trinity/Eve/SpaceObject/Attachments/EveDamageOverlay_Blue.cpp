// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveDamageOverlay.h"

#include "Curves/Fader/Tr2ScalarFader.h"
#include "Shader/Tr2Effect.h"
#include "TriSequencer.h"

BLUE_DEFINE( EveDamageOverlay );

const Be::ClassInfo* EveDamageOverlay::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDamageOverlay, "" )
		MAP_INTERFACE( EveDamageOverlay )

		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "seed", m_seed, "", Be::READWRITE )
		MAP_ATTRIBUTE( "damageLocatorCount", m_damageLocatorCount, "Number of damage locators to distribute auto-generated impacts over", Be::READWRITE )
		MAP_ATTRIBUTE( "configuration", m_configuration, "Impact goes into what?", Be::READ )
		MAP_ATTRIBUTE( "impactDataNextIdx", m_impactDataNextIdx, "", Be::READ )
		MAP_ATTRIBUTE( "armorImpactGoalCount", m_armorImpactGoalCount, "", Be::READ )
		MAP_ATTRIBUTE( "armorImpactParentSize", m_armorImpactParentSize, "", Be::READ )
		MAP_ATTRIBUTE( "armorImpactLifeTime", m_armorImpactLifeTime, "", Be::READWRITE )
		MAP_ATTRIBUTE( "debugForceSpawnDebris", m_debugForceSpawnDebris, "", Be::READWRITE )
		MAP_ATTRIBUTE( "renderPriority", m_renderPriority, "", Be::READ )

		MAP_ATTRIBUTE( "dataTextureBlockID", m_dataTextureBlockID, "The ID for our part in the big texture.", Be::READ )

		MAP_ATTRIBUTE( "armorDamageShader", m_armorDamageShader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorRepairing", m_armorRepairing, "", Be::READWRITE )
		MAP_ATTRIBUTE( "armorHardening", m_armorHardening, "", Be::READWRITE )

		MAP_ATTRIBUTE( "hullRepairing", m_hullRepairing, "", Be::READWRITE )

		MAP_ATTRIBUTE( "hullDamageFlickerCurve", m_hullDamageFlickerCurve, "This is the flickering for hull damage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullDamageFactor", m_hullDamageFactor, "How much hull damage to show?", Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"SetDamageState",
			SetDamageState,
			"Sets how much percentage is left of shield, armor and hull, optionally auto-creating armor impact holes.\n" )
		MAP_METHOD_AND_WRAP(
			"Clear",
			Clear,
			"Removes all impacts. Heals the object.\n" )
		MAP_METHOD_AND_WRAP(
			"ToggleEffect",
			ToggleEffect,
			"Toggles a named repair/hardening effect: armorhardening, armorrepair or hullrepair.\n" )
		MAP_METHOD_AND_WRAP(
			"CreateImpact",
			CreateImpact,
			"Creates an armor impact hole at the given damage locator index.\n" )
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
