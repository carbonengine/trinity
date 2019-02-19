#include "StdAfx.h"
#include "EveBoxVolume.h"

BLUE_DEFINE_INTERFACE( IEveVolume );

BLUE_DEFINE( EveBoxVolume );
const Be::ClassInfo* EveBoxVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveBoxVolume, "" )
		MAP_INTERFACE( EveBoxVolume )
		MAP_INTERFACE( IEveVolume )
		MAP_INTERFACE( INotify )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "innerScaling", m_innerScaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )

		EXPOSURE_END()
}