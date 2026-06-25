// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2DirectInstanceData.h"
#include "Utilities/BoundingBox.h"

BLUE_DEFINE( Tr2DirectInstanceData );

const Be::ClassInfo* Tr2DirectInstanceData::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DirectInstanceData, "" )
		MAP_INTERFACE( Tr2DirectInstanceData )
		MAP_INTERFACE( ITr2InstanceData )

		MAP_PROPERTY_READONLY(
			"count",
			GetCount,
			"Number of instances" )

		MAP_METHOD_AND_WRAP(
			"SetBoundingBox",
			SetBoundingBox,
			"Assign a bounding box explicitly" )

		MAP_ATTRIBUTE( "aabbMin", m_aabb.m_min, "Minimum of the AABB", Be::READ )

		MAP_ATTRIBUTE( "aabbMax", m_aabb.m_max, "Maximum of the AABB", Be::READ )

	EXPOSURE_END()
}