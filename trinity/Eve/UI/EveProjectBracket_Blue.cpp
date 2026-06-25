// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "EveProjectBracket.h"
#include "Sprite2d/Tr2Sprite2dContainer.h"

BLUE_DEFINE( EveProjectBracket );

const Be::ClassInfo* EveProjectBracket::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProjectBracket, "Projects a 3D position to 2D for brackets\n:jessica-deprecated: True" )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( EveProjectBracket )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name of this object",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"trackBall",
			m_trackBall,
			"Destiny ball used as source for bracket projection",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"ballTrackingScaling",
			m_ballTrackingScaling,
			"A scaling factor to apply to tracking balls",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"trackTransform",
			m_trackTransform,
			"The source position in 3D space. Note that 'trackBall' has priority.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"trackPosition",
			m_trackPosition,
			"Fixed position to track - used if neither trackBall or trackTransform are set.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"bracket",
			m_bracket,
			"The sprite container to receive the projected position",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"bracketIcon",
			m_bracketIcon,
			"The sprite to receive the projected position",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"parent",
			m_parent,
			"Parent of the sprite container",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"dock",
			m_dock,
			"Should the bracket dock on the sides if source is off the screen?"
			"\nIf not, the visible flag is toggled depending on off-screen status",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"marginLeft",
			m_marginLeft,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"marginRight",
			m_marginRight,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"marginTop",
			m_marginTop,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"marginBottom",
			m_marginBottom,
			"",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"minDispRange",
			m_minDispRange,
			"Bracket is hidden if the camera is closer to the object than this value",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"maxDispRange",
			m_maxDispRange,
			"Bracket is hidden if the camera is farther from the object than this value",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"cameraDistance",
			m_cameraDistance,
			"Distance of projected position to camera.",
			Be::READ )

		MAP_ATTRIBUTE(
			"offsetX",
			m_offsetX,
			"Horizontal offset for the bracket",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"offsetY",
			m_offsetY,
			"Vertical offset for the bracket",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"integerCoordinates",
			m_integerCoordinates,
			"If true (default), then projected coordinates are rounded to integer coordinates",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"projectedPosition",
			m_projectedPosition,
			"The projected 2d position.",
			Be::READ )
		MAP_ATTRIBUTE(
			"rawProjectedPosition",
			m_rawProjectedPosition,
			"The raw projected 2d position, without any clamping applied.",
			Be::READ )
		MAP_ATTRIBUTE(
			"displayChangeCallback",
			m_displayChangeCallback,
			"An optional callback that is called whenever the display state of the bracket\n"
			"is changed. It should be a callable, accepting one boolean argument (the new display state).",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"bracketUpdateCallback",
			m_bracketUpdateCallback,
			"An optional callback that is called whenever the bracket is updated while visible",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"isVisible",
			m_isVisible,
			"Visibility state of projected bracket",
			Be::READ )

		MAP_ATTRIBUTE(
			"isInFront",
			m_isInFront,
			"Flag telling if project bracket is infront or back of the camera",
			Be::READ )
	EXPOSURE_END()
}
