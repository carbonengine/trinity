////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildContainer.h"

BLUE_DEFINE( EveChildContainer );

const Be::ClassInfo* EveChildContainer::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildContainer, "" )
        MAP_INTERFACE( EveChildContainer )
		MAP_INTERFACE( IEveSpaceObjectChild )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "objects", m_objects, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling,"", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )

    EXPOSURE_END()
}