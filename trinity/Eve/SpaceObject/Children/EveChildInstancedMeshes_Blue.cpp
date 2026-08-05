// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildInstancedMeshes.h"



BLUE_DEFINE( EveChildInstancedMeshes );

const Be::ClassInfo* EveChildInstancedMeshes::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildInstancedMeshes, "" )
		MAP_INTERFACE( EveChildInstancedMeshes )
		MAP_INTERFACE( EveSpaceObjectChild )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveShadowCaster )
		MAP_INTERFACE( ITr2Renderable )

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
		MAP_METHOD_AND_WRAP(
			"GetMeshInheritOverlayEffects",
			GetMeshInheritOverlayEffects,
			"Returns True if the parent space object's overlay effects (e.g. cloak) also render over the given mesh\n\n"
			":param meshId: Index of the mesh to query\n"
			":rtype: bool" )
		MAP_METHOD_AND_WRAP(
			"SetMeshInheritOverlayEffects",
			SetMeshInheritOverlayEffects,
			"Sets whether the parent space object's overlay effects (e.g. cloak) also render over the given mesh.\n"
			"When False the mesh also ignores the parent's clip sphere so it stays visible while the rest of the ship dissolves.\n\n"
			":param meshId: Index of the mesh to modify\n"
			":param inherit: True to inherit the parent's overlay effects, False to opt out\n"
			":rtype: None" )
		MAP_METHOD_AND_WRAP(
			"AddMeshOverlayEffect",
			AddMeshOverlayEffect,
			"Adds an overlay effect owned by the given mesh. Rendered over the mesh's areas,\n"
			"underneath any overlay effects inherited from the parent space object.\n\n"
			":param meshId: Index of the mesh to modify\n"
			":param overlayEffect: The EveMeshOverlayEffect to add\n"
			":rtype: None" )
		MAP_METHOD_AND_WRAP(
			"RemoveMeshOverlayEffect",
			RemoveMeshOverlayEffect,
			"Removes the first matching overlay effect previously added to the given mesh.\n\n"
			":param meshId: Index of the mesh to modify\n"
			":param overlayEffect: The EveMeshOverlayEffect to remove\n"
			":rtype: None" )
		MAP_METHOD_AND_WRAP(
			"ClearMeshOverlayEffects",
			ClearMeshOverlayEffects,
			"Removes all overlay effects owned by the given mesh.\n\n"
			":param meshId: Index of the mesh to modify\n"
			":rtype: None" )
		MAP_METHOD_AND_WRAP(
			"GetMeshOverlayEffectCount",
			GetMeshOverlayEffectCount,
			"Returns the number of overlay effects owned by the given mesh.\n\n"
			":param meshId: Index of the mesh to query\n"
			":rtype: int" )

		MAP_PROPERTY_READONLY( "partTag", GetPartTag, "Part tag for multi-part space objects" )
		MAP_METHOD_AND_WRAP( "GetParent", GetParent, "Returns the parent space object child in the hierarchy" )
		MAP_METHOD_AND_WRAP( "GetOwner", GetOwner, "Returns the owner space object" )
	EXPOSURE_END()
}
