////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EvePlaneSetItem.h"

BLUE_DEFINE( EvePlaneSetItem );

const Be::ClassInfo* EvePlaneSetItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EvePlaneSetItem, "" )
        MAP_INTERFACE( EvePlaneSetItem )
		MAP_ATTRIBUTE( "name", m_name, "Standard name", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "position", m_position, "Translation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "Scaling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Rotation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "Color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Transform", m_layer1Transform, "Color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Transform", m_layer2Transform, "Color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Scroll", m_layer1Scroll, "Color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Scroll", m_layer2Scroll, "Color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "the bone index this plane is tight to\n:jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maskAtlasID", m_maskAtlasID, "the atlas ID for the mask map", Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}
