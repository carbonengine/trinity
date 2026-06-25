// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriLineSet.h"

BLUE_DEFINE( TriLineSet );


const Be::ClassInfo* TriLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriLineSet, "no comment" )
		MAP_INTERFACE( TriLineSet )

		MAP_METHOD_AND_WRAP( "Add", Add, "Add( to, tcolor, from, fcolor )\n"
										 ":param to:		start position in world space.\n"
										 ":param color:	integer value describing the color of the start of the line.\n"
										 ":param from_:	end position in world space.\n"
										 ":param fcolor:	integer value describing the color of the end of the line." )

		MAP_METHOD_AND_WRAP( "AddLines", AddLines, "AddLines(lines)\n"
												   "Add a list of lines to be drawn.\n"
												   ":param lines: The length of the list must be a multiple of two." )

		MAP_METHOD_AND_WRAP( "AddSphere", AddSphere, "AddSphere( center, radius, segments, color )\n"
													 ":param center:	location in worldspace.\n"
													 ":param radius:	float.\n"
													 ":param segments:	how many longitude and latitude lines to draw.\n"
													 ":param color:		integer value describing the color." )

		MAP_METHOD_AND_WRAP( "AddBox", AddBox, "AddBox( min, max, color )\n"
											   ":param min:		minimum bounds in world space.\n"
											   ":param max:		maximum bounds in world space.\n"
											   ":param color:	integer value describing the color." )

		MAP_METHOD_AND_WRAP( "Clear", Clear, "Clear()\n"
											 "Clear all the lines from the render list." )

		MAP_METHOD_AND_WRAP( "Render", RenderFromScript, "Render()\n"
														 "Render all the lines in the render list.\n"
														 "This must Be called in a render callback." )

		MAP_METHOD_AND_WRAP( "SetDefaultColor", SetDefaultColor, "SetDefaultColor( color )\n"
																 ":param color: The default Color to use when none is specified." )

		MAP_METHOD_AND_WRAP( "SetCurrentColor", SetCurrentColor, "SetCurrentColor( color )\n"
																 ":param color: The current color of the lines." )

		MAP_ATTRIBUTE( "transform", m_transform, "Transform to place and orient this lineset in the world", Be::READWRITE )
		MAP_ATTRIBUTE( "zEnable", m_zEnable, "Enable z-test for rendering.", Be::READWRITE )

	EXPOSURE_END()
}