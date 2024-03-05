#include "StdAfx.h"
#include "TriViewport.h"

BLUE_DEFINE( TriViewport );

const Be::ClassInfo* TriViewport::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriViewport, "Encapsulates the concept of a rendering viewport" )
		MAP_INTERFACE(TriViewport)

		MAP_ATTRIBUTE
		( 
			"x", 
			x, 
			"Left horizontal starting location for rasterization, "
			"measured in pixels from left edge of render target.", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"y", 
			y, 
			"Top vertical starting location for rasterization, "
			"meastured in pixels from top edge of render target.", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"width", 
			width, 
			"Number of pixels horizontally rasterized by the viewport", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"height", 
			height, 
			"Number of pixels vertically rasterized by the viewport", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"minZ", 
			minZ, 
			"Minimum depth of the viewport, must be between 0 and 1", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"maxZ", 
			maxZ, 
			"Maximum depth of the viewport, must be between 0 and 1", 
			Be::READWRITE | Be::PERSIST 
		)		

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__", 
			py__init__, 
			6,
			"Optional arguments are (int,int,int,int,float,float)\n"
			":param x: \n"
			":param y: \n"
			":param width: \n"
			":param height: \n"
			":param zn: \n"
			":param zf: \n"
			)

		MAP_METHOD_AND_WRAP( 
			"GetAspectRatio", 
			GetAspectRatio, 
			"Returns the aspect ratio for this viewport"
			)

	EXPOSURE_END()
}
