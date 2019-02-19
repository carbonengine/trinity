#include "StdAfx.h"
#include "EveChildEnvironment.h"

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
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "exclusionVolumes", m_exclusionVolumes, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}