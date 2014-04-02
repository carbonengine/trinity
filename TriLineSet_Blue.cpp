#include "StdAfx.h"
#include "TriLineSet.h"

BLUE_DEFINE( TriLineSet );


const Be::ClassInfo* TriLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriLineSet, "no comment")
		MAP_INTERFACE(TriLineSet)

		MAP_METHOD_AND_WRAP("Add", Add, 
				"Add( to, tcolor, from, fcolor )\n"
				"to:		start position in world space.\n"
				"tcolor:	integer value describing the color of the start of the line.\n"
				"from:	end position in world space.\n"
				"fcolor:	integer value describing the color of the end of the line."
				)

		MAP_METHOD_AND_WRAP("AddLines", AddLines, 
				"AddLines(list)\n"
				"Add a list of lines to be drawn.\n"
				"list: The length of the list must be a multiple of two."
				)

		MAP_METHOD_AND_WRAP("AddSphere", AddSphere, 
				"AddSphere( center, radius, segments, color )\n"
				"center:	location in worldspace.\n"
				"radius:	float.\n"
				"segments:	how many longitude and latitude lines to draw.\n"
				"color:		integer value describing the color."
				)

		MAP_METHOD_AND_WRAP("AddBox", AddBox, 
				"AddBox( min, max, color )\n"
				"min:		minimum bounds in world space.\n"
				"max:		maximum bounds in world space.\n"
				"color:	integer value describing the color."
				)

		MAP_METHOD_AND_WRAP("Clear", Clear, 
				"Clear()\n"
				"Clear all the lines from the render list."
				)

		MAP_METHOD_AND_WRAP("Render", RenderFromScript,
				"Render()\n"
				"Render all the lines in the render list.\n"
				"This must Be called in a render callback."
				)

		MAP_METHOD_AND_WRAP("SetDefaultColor", SetDefaultColor, 
				"SetDefaultColor( color )\n"
				"The default Color to use when none is specified."
				)

		MAP_METHOD_AND_WRAP("SetCurrentColor", SetCurrentColor, 
				"SetCurrentColor( color )\n"
				"The current color of the lines."
				)

		MAP_ATTRIBUTE( "transform", m_transform, "Transform to place and orient this lineset in the world", Be::READWRITE )
		MAP_ATTRIBUTE( "zEnable", m_zEnable, "Enable z-test for rendering.", Be::READWRITE )

	EXPOSURE_END()
}