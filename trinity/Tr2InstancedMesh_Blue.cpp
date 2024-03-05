////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2InstancedMesh.h"
#include "TriConstants.h"
#include "Resources/TriGeometryRes.h"

BLUE_DEFINE( Tr2InstancedMesh );
BLUE_DEFINE_INTERFACE( ITr2InstanceData );

namespace
{
	Be::VarChooser BoundsMethodChooser[] =
	{
		{ "STATIC", BeCast( Tr2InstancedMesh::STATIC ), "Bounds are defined explicitely on the mesh" },
		{ "DYNAMIC", BeCast( Tr2InstancedMesh::DYNAMIC ), "Bounds are defined by instance geometry and max instance size" },
		{ "DYNAMIC_SCALED", BeCast( Tr2InstancedMesh::DYNAMIC_SCALED ), "Bounds are defined by instance geometry and max instance size; instance size is scaled by geometry size" },
		{ 0 }
	};
}

BLUE_REGISTER_ENUM_EX( "Tr2InstanceMeshBoundsMethod", Tr2InstancedMesh::BoundsMethod, BoundsMethodChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2InstancedMesh::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InstancedMesh, "" )
		MAP_INTERFACE( Tr2InstancedMesh )

		MAP_ATTRIBUTE( "instanceGeometryResource", m_instanceGeometryResource, "na", Be::PERSISTONLY )			
		MAP_PROPERTY( "instanceGeometryResource", GetInstanceGeometryResource, SetInstanceGeometryRes, "na" )

		MAP_ATTRIBUTE_WITH_CHOOSER
		( 
			"instanceGeometryResPath", 
			m_instanceGeometryResPath, 
			"Resource path to granny file containing instance data", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST, 
			TriGR2Chooser
		)
		MAP_ATTRIBUTE( "instanceMeshIndex", m_instanceMeshIndex, "The index of the mesh within the instance granny file to use", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE_WITH_CHOOSER( "boundsMethod", m_boundsMethod, "Method for measuring mesh bounding box\n:jessica-group: Bounds", Be::READWRITE | Be::PERSIST | Be::ENUM, BoundsMethodChooser )
		MAP_ATTRIBUTE( "minBounds", m_minBounds, "Min bounds in local space for EXPLICIT bounds\n:jessica-group: Bounds", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxBounds", m_maxBounds, "Max bounds in local space for EXPLICIT bounds\n:jessica-group: Bounds", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxInstanceSize", m_maxInstanceSize, "Max instance radius for FROM_INSTANCES bounds\n:jessica-group: Bounds", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "instanceCount", m_instanceCount, "Buffer containing instance count for indirect rendering", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_CHAINTO( Tr2Mesh )
}

