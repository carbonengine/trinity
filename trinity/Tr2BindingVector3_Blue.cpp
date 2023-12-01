////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2BindingVector3.h"

BLUE_DEFINE( Tr2BindingVector3 );

const Be::ClassInfo* Tr2BindingVector3::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2BindingVector3, "" )
		MAP_INTERFACE( Tr2BindingVector3 )
		MAP_ATTRIBUTE( "value", m_value, "Vector3 value", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
