// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "EveUiObject.h"

BLUE_DEFINE( EveUiObject );

const Be::ClassInfo* EveUiObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveUiObject, "" )
		MAP_INTERFACE( EveUiObject )
		MAP_INTERFACE( IEveSpaceObject2 )

		MAP_ATTRIBUTE( "usePerspectiveScale", m_usePerspectiveScale, "Use distance-based scaling (perspective)", Be::READWRITE );

		MAP_METHOD_AND_WRAP( "GetNameForPickingAreaID", GetNameForPickingAreaID, "Get a unique name for the area ID provided by the scene picking\n"
																				 ":param areaID: index provided by the picking" )
		MAP_METHOD_AND_WRAP( "SetVisibilityForArea", SetVisibilityForArea, "Change visibility of an area of this object\n"
																		   ":param areaName: name of the area\n"
																		   ":param enable: enable or disable rendering" )

	EXPOSURE_CHAINTO( EveSpaceObject2 )
}