#include "StdAfx.h"
#include "Tr2MeshArea.h"

BLUE_DEFINE( Tr2MeshArea );




const Be::ClassInfo* Tr2MeshArea::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MeshArea, "" )

		MAP_INTERFACE( Tr2MeshArea )

		MAP_ATTRIBUTE( "name", m_name, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE )
		MAP_ATTRIBUTE( "index", m_index, "Start index of the area within the mesh to bind to this effect", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "count", m_count, "Number of areas within the mesh to bind to this effect", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "reversed", m_reversed, "Render mesh triangles in reverse order and with reversed culling order", Be::PERSISTONLY )
		MAP_PROPERTY( "reversed", IsReversed, SetReversed, "Render mesh triangles in reverse order and with reversed culling order" )
        MAP_ATTRIBUTE( "useSHLighting", m_useSHLighting, "Use SH lighting instead of full-forward lighting", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "effect", m_material, "Shader or Material effect", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

	EXPOSURE_END()
}
