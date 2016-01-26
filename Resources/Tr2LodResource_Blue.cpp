////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2LodResource.h"

BLUE_DEFINE( Tr2LodResource );

Be::VarChooser Tr2LodChooser[] =
{
	{
		"LOD_UNSPECIFIED",
		BeCast( TR2_LOD_UNSPECIFIED ),
		""
	},
	{
		"LOD_LOW",
		BeCast( TR2_LOD_LOW ),
		""
	},
	{
		"LOD_MEDIUM",
		BeCast( TR2_LOD_MEDIUM ),
		""
	},
	{
		"LOD_HIGH",
		BeCast( TR2_LOD_HIGH ),
		""
	},
	{
		"LOD_ULTRA",
		BeCast( TR2_LOD_ULTRA ),
		""
	},
	{
		"LOD_COUNT",
		BeCast( TR2_LOD_COUNT ),
		""
	},
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( 
	"Tr2LodResource", 
	Tr2Lod, 
	Tr2LodChooser, 
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2LodResource::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2LodResource, "Wrapper for resources with multiple levels of detail" )
		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"Name of this resource",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"ultraDetailResPath",
			m_resPath[TR2_LOD_ULTRA],
			"Resource path for high detail resource",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"highDetailResPath",
			m_resPath[TR2_LOD_HIGH],
			"Resource path for high detail resource",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"mediumDetailResPath",
			m_resPath[TR2_LOD_MEDIUM],
			"Resource path for medium detail resource",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"lowDetailResPath",
			m_resPath[TR2_LOD_LOW],
			"Resource path for low detail resource",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP
		(
			"SelectLod",
			SelectLod,
			"Select the level of detail for this resource"
		)

		MAP_PROPERTY_READONLY
		(
			"resource",
			GetResource,
			"Get the resource, according the selected level of detail"
		)
	EXPOSURE_END()
}