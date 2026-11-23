// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dEffect.h"

BLUE_DEFINE( Tr2Sprite2dEffect );

const Be::ClassInfo* Tr2Sprite2dEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dEffect, "" )
		MAP_INTERFACE( ITr2SpriteObject )
		MAP_INTERFACE( Tr2Sprite2dEffect )

		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"The effect drawn over the sprite's quad in place of the scene's shaders, or None. It gets the\n"
			"scene's vertices and constants (see Carbon/UI/UI.fxh), with texcoord 0 running 0 to 1 over the quad,\n"
			"texcoord 1 holding the quad's width and height, and the sprite's color in the vertex color.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"color",
			m_color,
			"Color of the sprite, passed to the effect as the vertex color. Its alpha is multiplied with its ancestors' opacity.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"glowBrightness",
			m_glowBrightness,
			"Glow brightness passed to the effect in the vertices. See spriteTarget.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"spriteTarget",
			m_target,
			"Render target passed to the effect in the vertices: either color or glow target",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteTargetChooser )
	EXPOSURE_CHAINTO( Tr2SpriteObjectBase )
}
