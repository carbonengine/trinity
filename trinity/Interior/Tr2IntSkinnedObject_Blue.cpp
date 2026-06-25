// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2IntSkinnedObject.h"

BLUE_DEFINE( Tr2IntSkinnedObject );

const Be::ClassInfo* Tr2IntSkinnedObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2IntSkinnedObject, "" )
		MAP_INTERFACE( Tr2IntSkinnedObject )
		MAP_INTERFACE( ITr2InteriorDynamic )
		MAP_INTERFACE( ITr2Interior )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( ITr2BoundingBox )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IBluePlacementObserver )

#if BLUE_WITH_PYTHON
		// bounding data
		MAPFLOATARRAYSIZE( "boundingSphereCenter", m_boundingSphere, BlueDefaultIID, "bounding sphere center", Be::READ, 3 )
#endif

		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere[3], "bounding sphere radius", Be::READ )

		MAP_ATTRIBUTE( "variableStore", m_variableStore, "Local variable store for this object", Be::READ )

		MAP_ATTRIBUTE( "depthOffset", m_depthOffset, "Depth offset for transparency sorting", Be::READWRITE | Be::PERSIST )
	EXPOSURE_CHAINTO( Tr2SkinnedObject )
}
