#include "StdAfx.h"
#include "Tr2Sprite2dStretch.h"

BLUE_DEFINE( Tr2Sprite2dStretch );

Be::VarChooser Tr2Sprite2dStretchDpiScaleBehaviorChooser[] =
{
	{
		"S2D_SSC_ALIGN_BOTTOMRIGHT",
		BeCast( S2D_SSC_ALIGN_BOTTOMRIGHT ),
		"Align to bottom/right (texture is not stretched vertically for horizontal stretch sprites"
		"or horizontally for vertical stretch sprites)"
	},
	{
		"S2D_SSC_ALIGN_TOPLEFT",
		BeCast( S2D_SSC_ALIGN_TOPLEFT ),
		"Align to top/left (texture is not stretched vertically for horizontal stretch sprites"
		"or horizontally for vertical stretch sprites)"
	},
	{
		"S2D_SSC_SCALE",
		BeCast( S2D_SSC_SCALE ),
		"Texture scales in both directions"
	},
	{ 0	}
};

BLUE_REGISTER_ENUM( "Tr2Sprite2dStretchDpiScaleBehaviorChooser", Tr2StretchScaleBehavior, Tr2Sprite2dStretchDpiScaleBehaviorChooser );

const Be::ClassInfo* Tr2Sprite2dStretch::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Sprite2dStretch, "" )
        MAP_INTERFACE( Tr2Sprite2dStretch )

		MAP_ATTRIBUTE
		(
			"leftEdgeSize", 
			m_leftEdgeSize, 
			"Number of fixed pixels on the left edge of the texture", 
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"rightEdgeSize", 
			m_rightEdgeSize, 
			"Number of fixed pixels on the right edge of the texture", 
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE(
			"edgeScale",
			m_edgeScale,
			"Edge size scaling when rendering the stretch; used for UI scaling",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE
		(
			"offset",
			m_offset,
			"Offset the sprite. Positive values will make it smaller horizontally,\n"
			"and negative bigger. The sprite is shifted vertically by this offset.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"fillCenter",
			m_fillCenter,
			"If set, the center of the sprite is filled (this is the default).",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"saturation",
			m_saturation,
			"Saturation factor, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"effectOpacity",
			m_effectOpacity,
			"Effect opacity, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"dpiScaleBehavior",
			m_dpiScaleBehavior,
			"Controls how the stretch behaves with dpi scaling.",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			Tr2Sprite2dStretchDpiScaleBehaviorChooser
		)

	EXPOSURE_CHAINTO( Tr2TexturedSpriteObject )
}
