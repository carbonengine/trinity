////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2Material.h"

BLUE_DEFINE( Tr2Material );


const Be::ClassInfo* Tr2Material::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Material, "" )
		MAP_INTERFACE( Tr2Material )
	EXPOSURE_END()
}