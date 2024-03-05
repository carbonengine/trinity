////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "EveSphereVolume.h"

BLUE_DEFINE( EveSphereVolume );
const Be::ClassInfo* EveSphereVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSphereVolume, "" )
		MAP_INTERFACE( EveSphereVolume )
		MAP_INTERFACE( IEveVolume )
		MAP_INTERFACE( INotify )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "radius", m_radius, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "innerRadius", m_innerRadius, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		EXPOSURE_END()
}