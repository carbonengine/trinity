////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveImpactOverlay.h"

BLUE_DEFINE( EveImpactOverlay );

const Be::ClassInfo* EveImpactOverlay::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveImpactOverlay, "" )
        MAP_INTERFACE( EveImpactOverlay )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST	)
		MAP_ATTRIBUTE( "seed", m_seed, "", Be::READ )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE )
		MAP_ATTRIBUTE( "configuration", m_configuration, "Impact goes into what?", Be::READ )
		MAP_ATTRIBUTE( "impactDataNextIdx", m_impactDataNextIdx, "", Be::READ )
		MAP_ATTRIBUTE( "armorImpactGoalCount", m_armorImpactGoalCount, "", Be::READ )
		MAP_ATTRIBUTE( "armorImpactParentSize", m_armorImpactParentSize, "", Be::READ )
		MAP_ATTRIBUTE( "shieldImpactColorFade", m_shieldImpactColorFade, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldImpactParentSize", m_shieldImpactParentSize, "", Be::READ )
		MAP_ATTRIBUTE( "shieldIsEllipsoid", m_shieldIsEllipsoid, "", Be::READWRITE )
		MAP_ATTRIBUTE( "debugForceSpawnDebris", m_debugForceSpawnDebris, "", Be::READWRITE )
		MAP_ATTRIBUTE( "renderPriority", m_renderPriority, "", Be::READ )

		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "dataTextureBlockID", m_dataTextureBlockID, "The ID for our part in the big texture.", Be::READ )

		MAP_ATTRIBUTE( "maxShieldImpacts", m_maxShieldImpacts, "", Be::READ )
		MAP_ATTRIBUTE( "overallShieldImpact", m_overallShieldImpact, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldHardening", m_shieldHardening, "", Be::READWRITE )
		MAP_ATTRIBUTE( "shieldBoosting", m_shieldBoosting, "", Be::READWRITE )

		MAP_ATTRIBUTE( "armorDamageShader", m_armorDamageShader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorImpactEmitter", m_armorImpactEmitter, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorRepairing", m_armorRepairing, "", Be::READWRITE )
		MAP_ATTRIBUTE( "armorHardening", m_armorHardening, "", Be::READWRITE )

		MAP_ATTRIBUTE( "hullRepairing", m_hullRepairing, "", Be::READWRITE )
		
		MAP_ATTRIBUTE( "hullDamageFlickerCurve", m_hullDamageFlickerCurve, "This is the flickering for hull damage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullDamageFactor", m_hullDamageFactor, "How much hull damage to show?", Be::READWRITE )
		MAP_ATTRIBUTE( "hullImpactEmitter", m_hullImpactEmitter, "The hull impact emitter", Be::READWRITE | Be::PERSIST )
		
		MAP_METHOD_AND_WRAP(
			"GetArmorImpactLifeTime",
			GetArmorImpactLifeTime,
			"Value for how long the overlay effect plays.\n"
		)

    EXPOSURE_END()
}