#include "StdAfx.h"
#include "EveChildMesh.h"

BLUE_DEFINE( EveChildMesh );

const Be::ClassInfo* EveChildMesh::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildMesh, "" )
        MAP_INTERFACE( EveChildMesh )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST	)
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useSpaceObjectData", m_useSpaceObjectData, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lowestLodVisible", m_lowestLodVisible, "Lowest LOD this guy is visible", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling,"", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform if useSRT is set." )

    EXPOSURE_END()
}