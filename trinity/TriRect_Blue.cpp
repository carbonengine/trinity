// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriRect.h"

#if BLUE_WITH_PYTHON

BLUE_DEFINE( TriRect );

const Be::ClassInfo* TriRect::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriRect, "" )
		MAP_INTERFACE( IPythonMethods )

		////////////////////////////////////////////////////////////////////////////
		//               left
		MAP_ATTRIBUTE(
			"left",
			left,
			"",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               top
		MAP_ATTRIBUTE(
			"top",
			top,
			"",
			Be::READWRITE | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		//               right
		MAP_ATTRIBUTE(
			"right",
			right,
			"",
			Be::READWRITE | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		//               bottom
		MAP_ATTRIBUTE(
			"bottom",
			bottom,
			"",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               __init__
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			SetDimentions,
			4,
			"Constructor arguments\n"
			":param left: \n"
			":param top: \n"
			":param right: \n"
			":param bottom: \n" )
		////////////////////////////////////////////////////////////////////////////
		//               SetRect
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"SetRect",
			PySetRect,
			4,
			":param left: \n"
			":param top: \n"
			":param right: \n"
			":param bottom: \n" )

	EXPOSURE_END()
}
#endif
