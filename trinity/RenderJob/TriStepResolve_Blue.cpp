// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepResolve.h"
#include "Tr2RenderTarget.h"

BLUE_DEFINE( TriStepResolve );

const Be::ClassInfo* TriStepResolve::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepResolve, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepResolve )

		MAP_ATTRIBUTE( "source", m_source, "Source of the resolve", Be::READWRITE )
		MAP_ATTRIBUTE( "destination", m_destination, "Target of the resolve", Be::READWRITE )
		MAP_ATTRIBUTE( "generateMipmap", m_generateMipmap, "Generate mipmaps in destination after the resolve?", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			2,
			"Creates a render step that resolves source into destination rendertargets. If source is not MSAA,\n"
			"this is a copy (but formats and dimensions must still match exactly).\n"
			":param destination: Tr2RenderTarget\n"
			":param source: Tr2RenderTarget" )

	EXPOSURE_CHAINTO( TriRenderStep )
}
