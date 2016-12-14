#include "StdAfx.h"

#include "TriStepRenderCubeMap.h"

BLUE_DEFINE( TriStepRenderCubeMap );

const Be::ClassInfo* TriStepRenderCubeMap::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderCubeMap, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderCubeMap )

		MAP_ATTRIBUTE( "texture", m_texture, "", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			py__init__,
			1,
			"Creates a render step that renders a cubemap on the screen for debugging\n"
			":param cubeMap: a TriTextureRes containing a cubemap (default None)"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}
