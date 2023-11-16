////////////////////////////////////////////////////////////
//
//    Creator:   Reevan McKay
//    Created:   December 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "Tr2MaterialRes.h"

BLUE_DEFINE( Tr2MaterialRes );

const Be::ClassInfo* Tr2MaterialRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MaterialRes, "" )
		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2MaterialRes )
		MAP_ATTRIBUTE( "name", m_name, "The name of the res", Be::READWRITE | Be::PERSIST ) 
		MAP_ATTRIBUTE( "meshes", m_meshes, "", Be::READ | Be::PERSIST ) 
	EXPOSURE_END()
}


BLUE_DEFINE( Tr2MaterialMesh );

const Be::ClassInfo* Tr2MaterialMesh::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MaterialMesh, "" )
		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2MaterialMesh )
		MAP_ATTRIBUTE( "areas", m_areas, "", Be::READ | Be::PERSIST ) 
	EXPOSURE_END()
}


BLUE_DEFINE( Tr2MaterialArea );

const Be::ClassInfo* Tr2MaterialArea::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2MaterialArea, "" )

        MAP_INTERFACE( IRoot )
        MAP_INTERFACE( Tr2MaterialArea )

        MAP_ATTRIBUTE( "metatype", m_metaType, "The metamaterial used to render this area", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "material", m_material, "The materialstore containing our overrides", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}
