// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dFrame.h"
#include "Tr2Sprite2dTexture.h"

BLUE_DEFINE( Tr2Sprite2dFrame );

const Be::ClassInfo* Tr2Sprite2dFrame::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dFrame, "" )
		MAP_INTERFACE( Tr2Sprite2dFrame )

		MAP_ATTRIBUTE(
			"offset",
			m_offset,
			"Offset the frame. Positive values will make it smaller, and negative bigger.",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"cornerSize",
			m_cornerSize,
			"Size of the corner of the frame",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"cornerScale",
			m_cornerScale,
			"Corner size scaling when rendering the frame; used for UI scaling",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"fillCenter",
			m_fillCenter,
			"If set, the center of the frame is filled (this is the default).",
			Be::READWRITE | Be::NOTIFY )

	EXPOSURE_CHAINTO( Tr2TexturedSpriteObject )
}
