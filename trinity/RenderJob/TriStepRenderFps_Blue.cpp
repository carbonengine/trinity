#include "StdAfx.h"

#include "TriStepRenderFps.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepRenderFps );

const Be::ClassInfo* TriStepRenderFps::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderFps, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderFps )

		MAP_ATTRIBUTE
		(
			"displayX",
			m_displayX,
			"x coordinate of fps numbers on screen. By default this is relative to\n"
			"the left edge of the viewport - if 'alignRight' is set, then this is\n"
			"relative to the right edge.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"displayY",
			m_displayY,
			"y coordinate of fps numbers on screen. By default this is relative to\n"
			"the top edge of the viewport - if 'alignBottom' is set, then this is\n"
			"relative to the bottom edge.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"alignRight",
			m_alignRight,
			"If set, then the the display of fps numbers is relative to the right edge\n"
			"of the viewport.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"alignBottom",
			m_alignBottom,
			"If set, then the the display of fps numbers is relative to the bottom edge\n"
			"of the viewport.",
			Be::READWRITE
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}
