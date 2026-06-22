// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dPickingMask.h"


BLUE_DEFINE( Tr2Sprite2dPickingMask );

namespace
{

const Be::VarChooser Channel_Chooser[] = {
	{ "Red", BeCast( 2 ), "" },
	{ "Green", BeCast( 1 ), "" },
	{ "Blue", BeCast( 0 ), "" },
	{ "Alpha", BeCast( 3 ), "" },
	{ 0 }
};

}

const Be::ClassInfo* Tr2Sprite2dPickingMask::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dPickingMask, "" )
		MAP_INTERFACE( Tr2Sprite2dPickingMask )

		MAP_PROPERTY(
			"maskPath",
			GetMaskPath,
			SetMaskPath,
			"Res path to the mask texture\n"
			":jessica-widget: filepath\n"
			":jessica-file-filter: texture" )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"channel",
			m_channel,
			"Map channel to use as a mask (0 - blue, 1 - green, 2 - red, 3 - alpha)",
			Be::READWRITE | Be::ENUM,
			Channel_Chooser )

		MAP_ATTRIBUTE(
			"threshold",
			m_threshold,
			"Mask map channel value threshold (from 0 to 1)",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"leftEdge",
			m_leftEdge,
			"Number of fixed pixels on the left edge of the texture",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"topEdge",
			m_topEdge,
			"Number of fixed pixels on the top edge of the texture",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"rightEdge",
			m_rightEdge,
			"Number of fixed pixels on the right edge of the texture",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"bottomEdge",
			m_bottomEdge,
			"Number of fixed pixels on the bottom edge of the texture",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"mask",
			m_mask,
			"Mask resource",
			Be::READ )
	EXPOSURE_END()
}
