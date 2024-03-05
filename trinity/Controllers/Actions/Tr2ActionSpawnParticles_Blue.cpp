////////////////////////////////////////////////////////////
//
//    Created:   September 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionSpawnParticles.h"


BLUE_DEFINE( Tr2ActionSpawnParticles );

const Be::ClassInfo* Tr2ActionSpawnParticles::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionSpawnParticles, "" )
		MAP_INTERFACE( Tr2ActionSpawnParticles )
		MAP_INTERFACE( ITr2ControllerAction )

		MAP_ATTRIBUTE( "emitter", m_emitter, "Destination emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rate", m_rate, "Emission rate / count", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
