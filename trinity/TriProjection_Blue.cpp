// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriProjection.h"

BLUE_DEFINE( TriProjection );

const Be::ClassInfo* TriProjection::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriProjection, "" )

		MAP_INTERFACE( TriProjection )
		MAP_INTERFACE( IRoot )

		MAP_METHOD_AND_WRAP(
			"PerspectiveFov",
			PerspectiveFov,
			"Set the transform to a Field of View perspective (LH)\n"
			":param fov: field of view angle\n"
			":param aspect: aspect ratio\n"
			":param zn: near plane distance\n"
			":param zf: far plane distance\n" )
		MAP_METHOD_AND_WRAP(
			"PerspectiveOffCenter",
			PerspectiveOffCenter,
			"Set the transform to an off center perspective view (LH)\n"
			":param left: left plane\n"
			":param right: right plane\n"
			":param bottom: bottom plane\n"
			":param top: top plane\n"
			":param zn: near plane distance\n"
			":param zf: far plane distance\n" )
		MAP_METHOD_AND_WRAP(
			"PerspectiveOrthographic",
			PerspectiveOrthographic,
			"Set the transform to an orthographic perspective view (LH)\n"
			":param width: projection width\n"
			":param height: projection height\n"
			":param zn: near plane distance\n"
			":param zf: far plane distance\n" )
		MAP_METHOD_AND_WRAP( "GetProjectionType", GetProjectionType, "1=Fov, 2=OffCenter, 3=Ortho, 4=Custom" )
		MAP_METHOD_AND_WRAP(
			"CustomProjection",
			CustomProjection,
			"Set a transform directly from a matrix\n"
			":param proj: projection transform matrix" )

		MAP_PROPERTY_READONLY( "transform", GetTransform, "Gets the transformation matrix according to current settings." )

	EXPOSURE_END()
}
