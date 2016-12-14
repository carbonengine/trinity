#include "StdAfx.h"
#include "TriStepRenderDebug.h"

BLUE_DEFINE( TriStepRenderDebug );

const Be::ClassInfo* TriStepRenderDebug::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderDebug, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderDebug )
		MAP_INTERFACE( ITr2DebugRenderer )

		MAP_ATTRIBUTE
		(
			"autoClear", m_autoClear, 
			"If set, all draw commands are cleared when the step is executed. Otherwise draw\n"
			"commands keep accumulating until Clear is explicitly called.",
			Be::READWRITE
		)
		MAP_METHOD_AND_WRAP
		( 
			"Print2D", Print2D, 
			"prints 'msg' at ('x', 'y') in the viewport, with the given color.\n"
			":param x: horizontal coordinate\n"
			":param y: vertical coordinate\n"
			":param color: text color\n"
			":param msg: message text\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"Print2Df", Print2Df, 
			"prints 'msg' formatted into the rectangle with upper-left coordinates of ('x', 'y'),\n"
			"width and height of 'w' and 'h', with the given color. The formatting is controlled\n"
			"by 'format'.\n"
			":param x: horizontal coordinate\n"
			":param y: vertical coordinate\n"
			":param width: text width\n"
			":param height: text height\n"
			":param format: formating flags\n"
			":param color: text color\n"
			":param msg: message text\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"Print3D", Print3D,
			"Print3D( pos, color, msg ) - prints msg projected from the given 3D coordinate,\n"
			"with the given color.\n"
			":param pos: 3D position\n"
			":param color: text color\n"
			":param msg: message text\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"DrawSphere", DrawSphere,
			"DrawSphere( center, radius, segments, color ) - draws a sphere as lines.\n"
			":param center: sphere center position\n"
			":param radius: sphere radius\n"
			":param segments: number of segments in the wireframe\n"
			":param color: sphere color\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawBox", DrawBox, 
			"DrawBox( min, max, color ) - draws a box as lines.\n"
			":param min: coordinates of the min corner\n"
			":param max: coordinates of the max corner\n"
			":param color: sphere color\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawCapsule", DrawCapsule,
			"DrawCapsule( start, end, radius, segments, color ) - draws a capsule\n"
			"(cylinder with rounded ends) as lines.\n"
			":param start: coordinates of the starting point of the cylinder\n"
			":param end: coordinates of the ending point of the cylinder\n"
			":param radius: capsule radius\n"
			":param segments: number of segments in the wireframe\n"
			":param color: capsule color\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawCylinder", DrawCylinder,
			"DrawCylinder( start, end, radius, segments, color ) - draws a cylinder as lines.\n"
			":param start: coordinates of the starting point of the cylinder\n"
			":param end: coordinates of the ending point of the cylinder\n"
			":param radius: cylinder radius\n"
			":param segments: number of segments in the wireframe\n"
			":param color: cylinder color\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawCone", DrawCone,
			"DrawCone( start, end, radius, segments, color ) - draws a cone as lines.\n"
			":param start: coordinates of the origin of the cone\n"
			":param end: coordinates of the ending point of the cone\n"
			":param radius: cone radius\n"
			":param segments: number of segments in the wireframe\n"
			":param color: cone color\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawLine", PyDrawLine,
			"DrawLine( from, fromColor, to, toColor ) - draws a line.\n"
			":param fromPos: line segment start\n"
			":param fromColor: color at line segment start\n"
			":param toPos: line segment end\n"
			":param toColor: color at line segment end\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"Clear", Clear,
			"Clears any accumulated draw commands."
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}