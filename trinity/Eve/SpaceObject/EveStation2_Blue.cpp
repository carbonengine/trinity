#include "StdAfx.h"
#include "EveStation2.h"

BLUE_DEFINE( EveStation2 );

const Be::ClassInfo* EveStation2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveStation2, "" )
        MAP_INTERFACE( EveStation2 )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )

    EXPOSURE_CHAINTO( EveSpaceObject2 )
}