// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriView.h"

BLUE_DEFINE( TriView );

const Be::ClassInfo* TriView::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriView, "" )

		MAP_INTERFACE( TriView )

		MAP_ATTRIBUTE( "transform", m_transform, "Transformation matrix", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"SetLookAtPosition",
			SetLookAtPosition,
			"Builds a view transform.  Takes (eyePosition, atPosition, upDirection) vectors as parameters.\n"
			":param eye: camera position\n"
			":param at: camera focus position\n"
			":param up: camera up direction\n" )

	EXPOSURE_END()
}