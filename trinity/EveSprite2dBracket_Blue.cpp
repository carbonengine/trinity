////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2012
// Copyright:	CCP 2012
//

#include "StdAfx.h"
#include "EveSprite2dBracket.h"

BLUE_DEFINE( EveSprite2dBracket );

const Be::ClassInfo* EveSprite2dBracket::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSprite2dBracket, "" )
		MAP_INTERFACE( EveSprite2dBracket )

		MAP_ATTRIBUTE
		(
			"display",
			m_display,
			"Is the bracket visible?",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"translation",
			m_translation,
			"Translation of this bracket.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"icon",
			m_icon,
			"Icon of this bracket.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"color",
			m_color,
			"Color of this bracket.",
			Be::READWRITE
		)

	EXPOSURE_END()
}