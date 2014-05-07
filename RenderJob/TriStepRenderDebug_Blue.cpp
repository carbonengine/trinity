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
			"Print2D( x, y, color, msg )\n"
			"  prints 'msg' at ('x', 'y') in the viewport, with the given color." 
		)
		MAP_METHOD_AND_WRAP
		( 
			"Print2Df", Print2Df, 
			"Print2Df( x, y, w, h, format, color, msg )\n"
			"  prints 'msg' formatted into the rectangle with upper-left coordinates of ('x', 'y'),\n"
			"  width and height of 'w' and 'h', with the given color. The formatting is controlled\n"
			"  by 'format'."
		)
		MAP_METHOD_AND_WRAP
		(
			"Print3D", Print3D,
			"Print3D( pos, color, msg ) - prints msg projected from the given 3D coordinate,\n"
			"with the given color."
		)
		MAP_METHOD_AND_WRAP
		( 
			"DrawSphere", DrawSphere,
			"DrawSphere( center, radius, segments, color ) - draws a sphere as lines."
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawBox", DrawBox, 
			"DrawBox( min, max, color ) - draws a box as lines."
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawCapsule", DrawCapsule,
			"DrawCapsule( start, end, radius, segments, color ) - draws a capsule\n"
			"(cylinder with rounded ends) as lines."
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawCylinder", DrawCylinder,
			"DrawCylinder( start, end, radius, segments, color ) - draws a cylinder as lines."
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawCone", DrawCone,
			"DrawCone( start, end, radius, segments, color ) - draws a cylinder as lines."
		)
		MAP_METHOD_AND_WRAP
		(
			"DrawLine", PyDrawLine,
			"DrawLine( from, fromColor, to, toColor ) - draws a line."
		)
		MAP_METHOD_AND_WRAP
		( 
			"Clear", Clear,
			"Clears any accumulated draw commands."
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}