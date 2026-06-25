// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderObject.h"

BLUE_DEFINE( TriStepRenderObject );

const Be::ClassInfo* TriStepRenderObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRenderObject, "Render step for rendering a scene" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderObject )

		MAP_ATTRIBUTE(
			"renderable",
			m_renderable,
			"The ITr2Renderable to be rendered",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"effectOverride",
			m_effectOverride,
			"Optional override effect",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "renderOpaque", m_typeEnabled[0], "render opaque batches?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "renderDecal", m_typeEnabled[1], "render decal batches?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "renderTransparent", m_typeEnabled[2], "render transparent batches?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "renderAdditive", m_typeEnabled[3], "render additive batches?", Be::READWRITE | Be::PERSIST )


		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Creates a render step that renders an ITr2Renderable\n"
			":param object: an ITr2Renderable object" )

	EXPOSURE_CHAINTO( TriRenderStep )
}