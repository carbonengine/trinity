#include "StdAfx.h"
#include "EveSpaceObjectDecal.h"

BLUE_DEFINE( EveSpaceObjectDecal );

const Be::ClassInfo* EveSpaceObjectDecal::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpaceObjectDecal, "" )
        MAP_INTERFACE( EveSpaceObjectDecal )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
        MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( ITr2Pickable )

		MAP_ATTRIBUTE( "name", m_name, "A name for this decal", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE )

		MAP_ATTRIBUTE( "position", m_position, "center of decal", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "orientation of decal", Be::READWRITE | Be::NOTIFY | Be::PERSIST );
		MAP_ATTRIBUTE( "scaling", m_scaling, "size of decal", Be::READWRITE | Be::NOTIFY | Be::PERSIST );

		MAP_ATTRIBUTE( "parentBoneIndex", m_parentBoneIndex, "the bone index this decal is tight to (-1 to disable)", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "minScreenSize", m_minScreenSize, "min size on screen in pixels before the decal is LODed out", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "decalEffect", m_decalEffect, "The effect used to draw the decal", Be::READWRITE | Be::PERSIST )

		MAP_PROPERTY_READONLY( "hasStaticIndexBuffers", HasStaticIndexBuffers, "" )
		MAP_METHOD_AND_WRAP( "GetDecalPrimitiveCounts", GetDecalPrimitiveCounts, "Returns an array of primitve counts" )
		MAP_METHOD_AND_WRAP(
			"GetStaticIndexBuffers",
			GetStaticIndexBuffers,
			"Returns the persisted index buffer. To be used by tools." )
	EXPOSURE_END()
}
