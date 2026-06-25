// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveSpriteSetItem.h"

BLUE_DEFINE( EveSpriteSetItem );

const Be::ClassInfo* EveSpriteSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpriteSetItem, "" )
		MAP_INTERFACE( EveSpriteSetItem )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"position",
			m_position,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE(
			"blinkRate",
			m_blinkRate,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"blinkPhase",
			m_blinkPhase,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"minScale",
			m_minScale,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"maxScale",
			m_maxScale,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"falloff",
			m_falloff,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"color",
			m_color,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE(
			"warpColor",
			m_warpColor,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE(
			"boneIndex",
			m_boneIndex,
			"the bone index this blinky is tight to\n:jessica-widget: boneindex",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
