// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dTextObject.h"
#include "Font/Tr2FontMeasurer.h"

BLUE_DEFINE( Tr2Sprite2dTextObject );

const Be::ClassInfo* Tr2Sprite2dTextObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dTextObject, "Tr2Sprite2dTextObject is used for rendering text." )
		MAP_PROPERTY(
			"fontMeasurer",
			GetFontMeasurer,
			SetFontMeasurer,
			"Font measurer to use for working with text" )

		MAP_ATTRIBUTE(
			"pickRadius",
			m_pickRadius,
			"Radius to use when picking. If radius is 0 the text object is treated as"
			"\na rectangular area.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"texturePrimary",
			m_texturePrimary,
			"Primary texture for this sprite",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"useSizeFromTexture",
			m_useSizeFromTexture,
			"If set, displayWidth/Height are ignored and size of primary texture is used instead.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textWidth",
			m_textWidth,
			"Width of clip rectangle to put around text",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textHeight",
			m_textHeight,
			"Height of clip rectangle to put around text",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"shadowSpriteEffect",
			m_shadowSpriteEffect,
			"Sprite effect to use with drop shadow if useShadowSpriteEffect is True\n"
			":jessica-group: Shadow",
			Be::READWRITE | Be::ENUM | Be::NOTIFY,
			Tr2SpriteObjectEffectChooser )

		MAP_ATTRIBUTE(
			"useShadowSpriteEffect",
			m_useShadowSpriteEffect,
			"If True, use shadowSpriteEffect for drop shadow, otherwise use spriteEffect\n"
			":jessica-group: Shadow",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"hasAuxiliaryTooltip",
			m_hasAuxiliaryTooltip,
			"Denotes that this label has auxiliary tooltips that should be passed back to the client when picking.",
			Be::READWRITE )

	EXPOSURE_CHAINTO( Tr2SpriteObject );
}