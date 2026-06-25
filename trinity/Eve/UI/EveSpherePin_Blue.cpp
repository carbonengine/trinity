// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveSpherePin.h"

BLUE_DEFINE( EveSpherePin );

const Be::ClassInfo* EveSpherePin::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpherePin, "" )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IEveTransform )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "A name for this pin", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "pinEffect", m_pinEffect, "The effect to use to draw the 3d pin", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "pickEffect", m_pickEffect, "The effect to use to draw the 3d pin\n:jessica-skip-validation:", Be::READ )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curvesets for animating things", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "geometryResPath", m_geomResPath, "resource path, should be the big sphere all the planets share!", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinEffectResPath", m_pinEffectResPath, "resource path to the drawing effect", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE( "sortValueMultiplier", m_sortValueMultiplier, "factor to definitely put pins with identical positions in the correct order", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enablePicking", m_enablePicking, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "uvAtlasScaleOffset", m_uvAtlasScaleOffset, "texture atlas support: xy = scale, zw = offset", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE( "centerNormal", m_centerNormal, "Normal in cartesian coords to the center of the pin on the sphere surface", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinRadius", m_pinRadius, "radius of the pin on the sphere surface in radians", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinMaxRadius", m_pinMaxRadius, "radius of the pin on the sphere surface in radians, this is the size used by the geometry and should be set as rarely as possible", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinRotation", m_pinRotation, "rotation of the pin on the sphere surface in radians", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinColor", m_pinColor, "color modulation", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_pinColor, "color modulation", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinAlphaThreshold", m_pinAlphaThreshold, "special alpha value that can be used to show a progress bar", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE( "primitiveCount", m_primitiveCount, "", Be::READ )

	EXPOSURE_END()
}
