// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dLine.h"
#include "Tr2AtlasTexture.h"

BLUE_DEFINE( Tr2Sprite2dLine );

const Be::ClassInfo* Tr2Sprite2dLine::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dLine, "" )
		MAP_INTERFACE( Tr2Sprite2dLine )

		MAP_ATTRIBUTE(
			"translationFrom",
			m_translationFrom,
			"Coordinate of starting point",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"translationTo",
			m_translationTo,
			"Coordinate of ending point",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"widthFrom",
			m_widthFrom,
			"Width of this line at the starting point",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"colorFrom",
			m_colorFrom,
			"Color of this line at the starting point",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"widthTo",
			m_widthTo,
			"Width of this line at the ending point",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textureWidth",
			m_textureWidth,
			"Width of the texture along the line",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textureOffsetBase",
			m_textureOffsetBase,
			"Base offset of the texture along the line. Can be used to reduce seam effect between textured lines",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textureOffset",
			m_textureOffset,
			"Offset of the texture along the line. Can be used for animations",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"colorTo",
			m_colorTo,
			"Color of this line at the ending point",
			Be::READWRITE | Be::NOTIFY )

	EXPOSURE_CHAINTO( Tr2TexturedSpriteObject )
}
