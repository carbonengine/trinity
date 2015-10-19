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
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "dataTextureBlockID", m_dataTextureBlockID, "The ID for our part in the big texture.", Be::READ )

		MAP_ATTRIBUTE( "maxShieldImpacts", m_maxShieldImpacts, "", Be::READ )
		MAP_ATTRIBUTE( "shieldEllipsoidCenter", m_shieldEllipsoidCenter, "", Be::READ )
		MAP_ATTRIBUTE( "shieldEllipsoidRadii", m_shieldEllipsoidRadii, "", Be::READ )
		MAP_ATTRIBUTE( "shieldImpactDataNextIdx", m_shieldImpactDataNextIdx, "", Be::READ )
		MAP_ATTRIBUTE( "overallShieldImpact", m_overallShieldImpact, "", Be::READWRITE )

		MAP_ATTRIBUTE( "armorImpactDataNextIdx", m_armorImpactDataNextIdx, "", Be::READ )

		MAP_ATTRIBUTE( "armorDamageShader", m_armorDamageShader, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}