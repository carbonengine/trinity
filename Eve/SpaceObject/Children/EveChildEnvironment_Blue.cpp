#include "StdAfx.h"
#include "EveChildEnvironment.h"


BLUE_DEFINE_INTERFACE( IEveVolume );

BLUE_DEFINE( EveSphereVolume );
const Be::ClassInfo* EveSphereVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSphereVolume, "" )
		MAP_INTERFACE( EveSphereVolume )
		MAP_INTERFACE( IEveVolume )
		MAP_INTERFACE( INotify )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		//MAP_ATTRIBUTE( "centerOffset", m_centerOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radius", m_radius, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "innerRadius", m_innerRadius, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}



BLUE_DEFINE( EveBoxVolume );
const Be::ClassInfo* EveBoxVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveBoxVolume, "" )
		MAP_INTERFACE( EveBoxVolume )
		MAP_INTERFACE( IEveVolume )
		MAP_INTERFACE( INotify )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		//MAP_ATTRIBUTE( "centerOffset", m_centerOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "innerScaling", m_innerScaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		
	EXPOSURE_END()
}


BLUE_DEFINE( EveChildEnvironment );

const Be::ClassInfo* EveChildEnvironment::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildEnvironment, "" )
		MAP_INTERFACE( EveChildEnvironment )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )		
		MAP_ATTRIBUTE( "intensity", m_environmentIntensity, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphere", m_boundingSphere, "", Be::READ )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "exclusionVolumes", m_exclusionVolumes, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}