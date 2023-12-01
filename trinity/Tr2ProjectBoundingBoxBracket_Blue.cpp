//////////////////////////////////////////////////////////////////////////
//
// Created: March 2011
// Copyright CCP 2011
//
#include "StdAfx.h"

#include "Tr2ProjectBoundingBoxBracket.h"
#include "Sprite2d/Tr2Sprite2dContainer.h"

BLUE_DEFINE( Tr2ProjectBoundingBoxBracket );

const Be::ClassInfo* Tr2ProjectBoundingBoxBracket::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ProjectBoundingBoxBracket, "Projects a 3D bounding box to 2D for brackets \n:jessica-deprecated: True" )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( Tr2ProjectBoundingBoxBracket )
		
		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"Name of this object",
			Be::READWRITE
		)
		
		MAP_ATTRIBUTE
		(
			"object",
			m_object,
			"Object to track",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"bracket",
			m_bracket,
			"The sprite container to receive the projected position",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"parent",
			m_parent,
			"Parent of the sprite container",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"minProjectedWidth",
			m_minProjectedWidth,
			"Minimum width after projection",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"minProjectedHeight",
			m_minProjectedHeight,
			"Minimum height after projection",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"maxProjectedWidth",
			m_maxProjectedWidth,
			"Maximum width after projection",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"maxProjectedHeight",
			m_maxProjectedHeight,
			"Maximum height after projection",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"projectedX",
			m_projectedX,
			"x-coordinate after projection",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"projectedY",
			m_projectedY,
			"y-coordinate after projection",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"projectedZ",
			m_projectedZ,
			"z-coordinate after projection",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"projectedWidth",
			m_projectedWidth,
			"Width after projection",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"projectedHeight",
			m_projectedHeight,
			"Height after projection",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"cameraDistance",
			m_cameraDistance,
			"Distance of the object from the camera, as determined by the\n"
			"center of the bounding box.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"integerCoordinates",
			m_integerCoordinates,
			"If set, projected x, y, width and height are rounded to the nearest integer.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"screenMargin",
			m_screenMargin,
			"Brackets are never projected outside the screen - this controls the margin from"
			"\nthe edges of the screen.",
			Be::READWRITE
		)

	EXPOSURE_END()
}
