////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshLod.h"

BLUE_DEFINE( Tr2MeshLod );

const Be::ClassInfo* Tr2MeshLod::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MeshLod, "A mesh with levels of detail" )
		MAP_ATTRIBUTE
		(
			"geometryRes",
			m_geometryRes,
			"Geometry LOD resource for this mesh",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"associatedResources", 
			m_associatedResources, 
			"List of resources associated with this mesh that can select level of detail", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"selectedLod",
			m_selectedLod,
			"Currently selected level of detail",
			Be::READ
		)

		MAP_METHOD_AND_WRAP
		(
			"SelectLod",
			SelectLod,
			"Selects the level of detail on all resources associated with this mesh"
		)

	EXPOSURE_CHAINTO( Tr2MeshBase )
}