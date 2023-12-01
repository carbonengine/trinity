#include "StdAfx.h"
#include "TriFloat.h"

BLUE_DEFINE( TriFloat );

const Be::ClassInfo* TriFloat::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriFloat, "" )
        MAP_INTERFACE( TriFloat )

		MAP_ATTRIBUTE( "value", m_value, "", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}
