// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildInstancedMeshes.h"



BLUE_DEFINE( EveChildInstancedMeshes );

const Be::ClassInfo* EveChildInstancedMeshes::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildInstancedMeshes, "" )
		MAP_INTERFACE( EveChildInstancedMeshes )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveShadowCaster )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"GetSofSourceLocator",
			GetSofSourceLocator,
			"Returns SOF source locator information for the mesh given the picked areaID.\n"
			"Returns a tuple (SOF hull name, locator set name, locator index) or None if no SOF source is found.\n"
			"For the method to work the space object should have been built using SOF in the editor mode.\n\n"
			":param areaId: The areaID returned from mouse picking method\n"
			":rtype: None | (str, str, int)" )

		MAP_METHOD_AND_WRAP(
			"GetMeshCount",
			GetMeshCount,
			"Returns the number of instanced meshes in this object." )
		MAP_METHOD_AND_WRAP(
			"GetMeshInfo",
			GetMeshInfo,
			"Returns information about the instanced mesh at the given index. Returns a tuple with\n"
			"geometry res path, geometry res, mesh index, casts shadow flag, reflection mode, number of areas, and number of instances\n\n"
			":param meshId: Index of the mesh to query\n"
			":rtype: (str, trinity.TriGeometryRes, int, bool, int, int, int)" )
		MAP_METHOD_AND_WRAP(
			"GetAreaInfo",
			GetAreaInfo,
			"Returns information about the area at the given index in the given mesh. Returns a tuple with\n"
			"effect, batch type, area index, area count\n\n"
			":param meshId: Index of the mesh to query\n"
			":param areaId: Index of the area to query\n"
			":rtype: (trinity.Tr2Effect, int, int, int)" )
		MAP_METHOD_AND_WRAP(
			"GetMeshDisplay",
			GetMeshDisplay,
			"Returns True if the mesh is rendered, False otherwise\n\n"
			":param meshId: Index of the mesh to query\n"
			":rtype: bool" )
		MAP_METHOD_AND_WRAP(
			"SetMeshDisplay",
			SetMeshDisplay,
			"Sets whether the mesh is rendered or not\n\n"
			":param meshId: Index of the mesh to modify\n"
			":param display: True to render the mesh, False to hide it\n"
			":rtype: None" )

	EXPOSURE_END()
}