// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2d.h"
#include "Tr2AtlasTexture.h"

BLUE_DEFINE( Tr2Sprite2d );

const Be::ClassInfo* Tr2Sprite2d::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2d, "" )
		MAP_INTERFACE( Tr2Sprite2d )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"pickRadius",
			m_pickRadius,
			"Radius to use when picking. If radius is 0 the sprite is treated as"
			"\na rectangular area.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"useSizeFromTexture",
			m_useSizeFromTexture,
			"If set, displayWidth/Height are ignored and size of primary texture is used instead.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"saturation",
			m_saturation,
			"Saturation factor, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"effectOpacity",
			m_effectOpacity,
			"Effect opacity, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used.",
			Be::READWRITE | Be::NOTIFY )
	EXPOSURE_CHAINTO( Tr2TexturedSpriteObject )
}
