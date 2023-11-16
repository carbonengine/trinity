#include "StdAfx.h"
#include "Tr2BoundingLineSet.h"

BLUE_DEFINE( Tr2BoundingLineSet );

const Be::ClassInfo* Tr2BoundingLineSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2BoundingLineSet, "" )
        MAP_INTERFACE( Tr2BoundingLineSet )
		MAP_ATTRIBUTE( "minBounds", m_minBounds, 
			"Minimum bounds of the box",  
			Be::READWRITE| Be::NOTIFY| Be::PERSIST  
		)
		MAP_ATTRIBUTE( "maxBounds", m_maxBounds, 
			"Maximum bounds of the box", 
			Be::READWRITE| Be::NOTIFY| Be::PERSIST 
		)
		MAP_METHOD_AND_WRAP( "UpdateBounds", UpdateBounds, 
			"( min, max )\n"
			"Update the bounding volume\n" 
			":param boundsMin: min values for bounding box\n"
			":param boundsMax: max values for bounding box\n"
		)
    EXPOSURE_CHAINTO( Tr2LineSet )
}
