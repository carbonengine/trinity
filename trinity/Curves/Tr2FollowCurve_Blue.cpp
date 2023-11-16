////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2FollowCurve.h"

BLUE_DEFINE( Tr2FollowCurve );

const Be::ClassInfo* Tr2FollowCurve::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FollowCurve, "" )
		MAP_INTERFACE( ITriVectorFunction )
		MAP_INTERFACE( ITriFunction )

		MAP_ATTRIBUTE( "name", m_name, "The name of the curve", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "keys", m_keys, "The keys of the curve", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "currentValue", m_currentValue, "The current value of the curve", Be::READ )
		
	EXPOSURE_END()
}