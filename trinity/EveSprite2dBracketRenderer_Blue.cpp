////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2012
// Copyright:	CCP 2012
//

#include "StdAfx.h"
#include "EveSprite2dBracketRenderer.h"
#include "Tr2AtlasTexture.h"

BLUE_DEFINE( EveSprite2dBracketRenderer );

const Be::ClassInfo* EveSprite2dBracketRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSprite2dBracketRenderer, "" )
		MAP_INTERFACE( EveSprite2dBracketRenderer )

		MAP_ATTRIBUTE
		(
			"brackets",
			m_brackets,
			"The list of individual brackets",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"iconAtlas",
			m_iconAtlas,
			"A reference to a texture in the atlas that all icons are assumed to live in.",
			Be::READWRITE
		)

	EXPOSURE_CHAINTO( Tr2SpriteObject )
}