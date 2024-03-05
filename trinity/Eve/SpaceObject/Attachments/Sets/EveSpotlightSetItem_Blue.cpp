////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "EveSpotlightSetItem.h"

BLUE_DEFINE( EveSpotlightSetItem );

const Be::ClassInfo* EveSpotlightSetItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSpotlightSetItem, "" )
        MAP_INTERFACE( EveSpotlightSetItem )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_transform, "The tranform matrix for the item", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "coneColor", m_coneColor, "The color of the cone of light", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteColor", m_spriteColor, "The color of the glow at the base of the spotlight", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flareColor", m_flareColor, "The color of the flare", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "the bone index this spotlight is tight to\n:jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boosterGainInfluence", m_boosterGainInfluence, "Toggles if this spotlight can be used to show ship movement speed", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE
		( 
			"spriteScale", 
			m_spriteScale, 
			"This scale of the glow and flare at the base of the spotlight. First value is the uniform glow and the other two control the flare in 2D.", 
			Be::READWRITE | Be::PERSIST 
		)

    EXPOSURE_END()
}
