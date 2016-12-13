#include "StdAfx.h"

#include "EveShip2Builder.h"
#include "SpaceObject/EveShip2.h"

BLUE_DEFINE( EveShip2Builder );

const Be::ClassInfo* EveShip2Builder::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveShip2Builder, "" )
        MAP_INTERFACE( EveShip2Builder )

		MAP_ATTRIBUTE_WITH_CHOOSER( "electronic", m_moduleResPath[0], "", Be::READWRITE | Be::PERSIST, NULL )
		MAP_ATTRIBUTE_WITH_CHOOSER( "defensive", m_moduleResPath[1], "", Be::READWRITE | Be::PERSIST, NULL )
		MAP_ATTRIBUTE_WITH_CHOOSER( "engineering", m_moduleResPath[2], "", Be::READWRITE | Be::PERSIST, NULL )
		MAP_ATTRIBUTE_WITH_CHOOSER( "offensive", m_moduleResPath[3], "", Be::READWRITE | Be::PERSIST, NULL )
		MAP_ATTRIBUTE_WITH_CHOOSER( "propulsion", m_moduleResPath[4], "", Be::READWRITE | Be::PERSIST, NULL )

		MAP_ATTRIBUTE( "highDetailOutputName", m_highDetailOutputName, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weldThreshold", m_weldThreshold, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "PrepareForBuild", PrepareForBuild, "Loads the modules and initiates loading of resources used by them." )
		MAP_METHOD_AND_WRAP( "Build", Build, "Builds the final ship. Assumes resources have finished loading." )
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP( 
			"BuildAsync", 
			BuildAsync, 
			"Builds the final ship on a background thread. Assumes resources have finished loading.\n"
			":param cb: callback function that is called when the build is finished"
			)
#endif


		MAP_METHOD_AND_WRAP( "GetShip", GetShip, "Returns the results from Build." )

    EXPOSURE_END()
}
