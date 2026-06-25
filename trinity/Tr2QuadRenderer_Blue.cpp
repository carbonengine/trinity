// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2QuadRenderer.h"


BLUE_DEFINE( Tr2QuadRenderer );

const Be::ClassInfo* Tr2QuadRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2QuadRenderer, "" )
		MAP_INTERFACE( Tr2QuadRenderer )

		MAP_PROPERTY_READONLY(
			"instanceBufferSize",
			GetInstanceBufferSize,
			"Current size of the instance buffer" )

		MAP_PROPERTY_READONLY(
			"instanceDataSize",
			GetInstanceDataSize,
			"Size of the instance data last frame" )

	EXPOSURE_END()
}