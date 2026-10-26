// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2Mesh.h"

#include "Resources/TriGeometryRes.h"
#include "TriConstants.h"


BLUE_DEFINE( Tr2SerializedMorphAnimation );
const Be::ClassInfo* Tr2SerializedMorphAnimation::ExposeToBlue(){
	EXPOSURE_BEGIN( Tr2SerializedMorphAnimation, "" )
		MAP_INTERFACE( Tr2SerializedMorphAnimation )
			MAP_ATTRIBUTE( "name", m_name, "", Be::PERSISTONLY )
				MAP_ATTRIBUTE( "weight", m_weight, "", Be::PERSISTONLY )
					EXPOSURE_END()
}


BLUE_DEFINE( Tr2Mesh );


const Be::ClassInfo* Tr2Mesh::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Mesh, "" )

		MAP_INTERFACE( Tr2Mesh )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "geometry", m_geometryResource, "na", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"geometryResPath",
			m_meshResPath,
			"Resource path to granny file",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST,
			TriGR2Chooser )
		MAP_ATTRIBUTE( "deferGeometryLoad", m_deferGeometryLoad, "Defers geometry load when loading mesh objects", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"SetGeometryRes",
			PySetGeometryRes,
			"Set the geometry resource used by this mesh - bypassing the regular method of setting a resource name.\n"
			"This is used for geometry resources that require special handling, such as pre-baked blendshapes.\n"
			":param geometry: new geometry resource" )

		MAP_ATTRIBUTE( "serializedMorphAnimations", m_serializedMorphAnimations, "", Be::PERSISTONLY )

	EXPOSURE_CHAINTO( Tr2MeshBase )
}
