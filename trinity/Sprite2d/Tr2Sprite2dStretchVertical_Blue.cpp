// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dStretchVertical.h"

BLUE_DEFINE( Tr2Sprite2dStretchVertical );

const Be::ClassInfo* Tr2Sprite2dStretchVertical::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dStretchVertical, "" )
		MAP_INTERFACE( Tr2Sprite2dStretchVertical )

		MAP_ATTRIBUTE(
			"topEdgeSize",
			m_topEdgeSize,
			"Number of fixed pixels on the top edge of the texture",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"bottomEdgeSize",
			m_bottomEdgeSize,
			"Number of fixed pixels on the bottom edge of the texture",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"edgeScale",
			m_edgeScale,
			"Edge size scaling when rendering the stretch; used for UI scaling",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"fillCenter",
			m_fillCenter,
			"If set, the center of the sprite is filled (this is the default).",
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
