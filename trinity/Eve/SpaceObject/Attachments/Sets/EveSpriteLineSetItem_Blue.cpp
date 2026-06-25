// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "EveSpriteLineSetItem.h"

BLUE_DEFINE( EveSpriteLineSetItem );

const Be::ClassInfo* EveSpriteLineSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpriteLineSetItem, "" )
		MAP_INTERFACE( EveSpriteLineSetItem )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isCircle", m_isCircle, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spacing", m_spacing, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkRate", m_blinkRate, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkPhase", m_blinkPhase, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkPhaseShift", m_blinkPhaseShift, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minScale", m_minScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxScale", m_maxScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
