// Copyright © 2011 CCP ehf.

#include "StdAfx.h"

#include "Tr2ProjectBoundingBoxBracket.h"
#include "Sprite2d/Tr2Sprite2dContainer.h"

BLUE_DEFINE( Tr2ProjectBoundingBoxBracket );

const Be::ClassInfo* Tr2ProjectBoundingBoxBracket::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ProjectBoundingBoxBracket, "Projects a 3D bounding box to 2D for brackets" )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( Tr2ProjectBoundingBoxBracket )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name of this object",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"object",
			m_object,
			"Object to track",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"bracket",
			m_bracket,
			"The sprite container to receive the projected position",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"parent",
			m_parent,
			"Parent of the sprite container",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"minProjectedWidth",
			m_minProjectedWidth,
			"Minimum width after projection. Not applied while containsCamera\n"
			"is true: the rect is the full viewport inset by screenMargin.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"minProjectedHeight",
			m_minProjectedHeight,
			"Minimum height after projection. Not applied while containsCamera\n"
			"is true: the rect is the full viewport inset by screenMargin.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"maxProjectedWidth",
			m_maxProjectedWidth,
			"Maximum width after projection. Not applied while containsCamera\n"
			"is true: the rect is the full viewport inset by screenMargin.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"maxProjectedHeight",
			m_maxProjectedHeight,
			"Maximum height after projection. Not applied while containsCamera\n"
			"is true: the rect is the full viewport inset by screenMargin.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"projectedX",
			m_projectedX,
			"x-coordinate after projection",
			Be::READ )
		MAP_ATTRIBUTE(
			"projectedY",
			m_projectedY,
			"y-coordinate after projection",
			Be::READ )
		MAP_ATTRIBUTE(
			"projectedZ",
			m_projectedZ,
			"z-coordinate after projection",
			Be::READ )
		MAP_ATTRIBUTE(
			"projectedWidth",
			m_projectedWidth,
			"Width after projection",
			Be::READ )
		MAP_ATTRIBUTE(
			"projectedHeight",
			m_projectedHeight,
			"Height after projection",
			Be::READ )
		MAP_ATTRIBUTE(
			"cameraDistance",
			m_cameraDistance,
			"Distance of the object from the camera, as determined by the\n"
			"center of the bounding box. Measured in the space the object is\n"
			"rendered in: render-scaled objects (e.g. planets) report the\n"
			"distance to their scaled proxy, not the true world distance.",
			Be::READ )

		MAP_ATTRIBUTE(
			"integerCoordinates",
			m_integerCoordinates,
			"If set, projected x, y, width and height are rounded to the nearest integer.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"screenMargin",
			m_screenMargin,
			"If greater than zero, the projected rect is clamped inside the viewport"
			"\ninset by this many pixels, and the projection becomes empty when nothing"
			"\nremains inside that safe frame. Zero (default) disables clamping.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"isProjectionValid",
			m_isProjectionValid,
			"True when the bounding box produced a valid projected rect.",
			Be::READ )

		MAP_ATTRIBUTE(
			"containsCamera",
			m_containsCamera,
			"True when the camera is inside the tracked bounding box.",
			Be::READ )

		MAP_ATTRIBUTE(
			"extendsOffscreen",
			m_extendsOffscreen,
			"True when the raw projection of the bounding box extends beyond the\n"
			"current viewport. Describes the projection itself, not the output rect:\n"
			"min/max sizing, recentering and screenMargin clamping do not affect\n"
			"this flag. Compare projectedX/Y/Width/Height against the viewport if\n"
			"you need the output rect's overlap instead. Always true when\n"
			"containsCamera is true.",
			Be::READ )

		MAP_ATTRIBUTE(
			"coversViewport",
			m_coversViewport,
			"True when the raw projection of the bounding box covers the entire\n"
			"viewport. Describes the projection itself, not the output rect:\n"
			"min/max sizing, recentering and screenMargin clamping do not affect\n"
			"this flag. Always true when containsCamera is true.",
			Be::READ )

	EXPOSURE_END()
}
