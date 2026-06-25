// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepGenerateMipMaps.h"
#include "TriRenderStep.h"


BLUE_DEFINE( TriStepGenerateMipMaps );


const Be::ClassInfo* TriStepGenerateMipMaps::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepGenerateMipMaps, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepGenerateMipMaps )

		MAP_ATTRIBUTE( "renderTarget", m_renderTarget, "na", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Creates a render job that generates mipmaps. Note that the target of a TriStepResolve\n"
			"generates the mipmaps, so use this only with non-MSAA targets.\n"
			":param renderTarget: a Tr2RenderTarget" )

	EXPOSURE_CHAINTO( TriRenderStep )
}