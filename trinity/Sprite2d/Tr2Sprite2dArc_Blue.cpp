// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dArc.h"

BLUE_DEFINE( Tr2Sprite2dArc );

const Be::ClassInfo* Tr2Sprite2dArc::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dArc, "" )
		MAP_INTERFACE( Tr2Sprite2dArc )

		MAP_ATTRIBUTE(
			"radius",
			m_radius,
			"Radius",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"lineWidth",
			m_lineWidth,
			"Width of line rendered",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"lineColor",
			m_lineColor,
			"Color of line rendered",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"startAngle",
			m_startAngle,
			"Start angle of the arc",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"endAngle",
			m_endAngle,
			"Ending angle of the arc",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"fill",
			m_fill,
			"If set, the arc is filled - otherwise only a line is drawn",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"texturePrimary",
			m_texturePrimary,
			"Primary texture for this object",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textureSecondary",
			m_textureSecondary,
			"Secondary texture for this object",
			Be::READWRITE | Be::NOTIFY )
	EXPOSURE_CHAINTO( Tr2SpriteObject )
}
