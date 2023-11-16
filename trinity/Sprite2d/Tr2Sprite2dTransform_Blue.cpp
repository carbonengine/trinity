#include "StdAfx.h"
#include "Tr2Sprite2dTransform.h"

BLUE_DEFINE( Tr2Sprite2dTransform );

// This is defined in Tr2SpriteObject_Blue.cpp
extern Be::VarChooser Tr2SpriteObjectPickStateChooser[];

const Be::ClassInfo* Tr2Sprite2dTransform::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Sprite2dTransform, "" )

		MAP_ATTRIBUTE
		(
			"rotationCenter",
			m_rotationCenter,
			"Center point for rotation applied to children of this transform",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"rotation",
			m_rotation,
			"Rotation applied to children of this transform",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"scalingCenter",
			m_scalingCenter,
			"Center point for scale applied to children of this transform",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"scalingRotation",
			m_scalingRotation,
			"Scaling rotation factor for scale applied to children of this transform",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"scale",
			m_scale,
			"Scale applied to children of this transform",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_METHOD_AND_WRAP
		(
			"TransformPoint",
			TransformPoint,
			"Get transformed result of point x, y\n"
			":param x: horizontal coordinate\n"
			":param y: vertical coordinate\n"
		)

	EXPOSURE_CHAINTO( Tr2Sprite2dContainerBase )
}
