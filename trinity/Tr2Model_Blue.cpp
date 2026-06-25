// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Model.h"

BLUE_DEFINE( Tr2Model );

const Be::ClassInfo* Tr2Model::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Model, "" )

		MAP_INTERFACE( Tr2Model )

		MAP_ATTRIBUTE( "name", m_name, "Name of this model", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "meshes", m_meshes, "Meshes are sorted relative to the center of their granny mesh when getting transparent batches", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"GetBoundingBoxInLocalSpace",
			GetBoundingBoxInLocalSpace,
			"Gets the bounding box in local space" )

	EXPOSURE_END()
}
