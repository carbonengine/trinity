// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dLayer.h"

BLUE_DEFINE( Tr2Sprite2dLayer );

const Be::ClassInfo* Tr2Sprite2dLayer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dLayer, "" )
		MAP_INTERFACE( Tr2Sprite2dLayer )

		MAP_ATTRIBUTE(
			"backgroundColor",
			m_backgroundColor,
			"Background color for this layer",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"color",
			m_color,
			"Color for this layer",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"clearBackground",
			m_clearBackground,
			"If set, background is cleared to 'backgroundColor'",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"blendMode",
			m_blendMode,
			"Blendmode controls how the object blends with the background",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteObjectBlendModeChooser )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"spriteEffect",
			m_spriteEffect,
			"Sprite effect controls how textures are used to render the object",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteObjectEffectChooser )

		MAP_PROPERTY(
			"textureSecondary",
			GetTextureSecondary,
			SetTextureSecondary,
			"Secondary texture for this sprite" )

		MAP_ATTRIBUTE(
			"children",
			m_children,
			"Children of this Layer",
			Be::READ | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2Sprite2dContainer )
}
