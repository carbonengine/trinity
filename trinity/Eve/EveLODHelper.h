// Copyright © 2012 CCP ehf.

#pragma once
#ifndef EveLODHelper_h
#define EveLODHelper_h

#include "Resources/Tr2LodResource.h"
#include "Eve/EveUpdateContext.h"

// externs
extern float g_eveSpaceSceneLowUpdateRate;
extern float g_eveSpaceSceneMediumUpdateRate;


enum BoundingSphereQuery
{
	EVE_BOUNDS_NORMAL,
	EVE_BOUNDS_WITH_CHILDREN
};

class EveLODHelper
{
public:
	// --------------------------------------------------------------------------------
	// Description:
	//   Determines weather an object should be updated depending on LOD and time since
	//   last update
	// Arguments:
	//   lod - an object's LOD
	//   timeSinceUpdate - time since last update in seconds
	// Return value:
	//   Returns true if caller should be updated.
	// --------------------------------------------------------------------------------
	static inline bool ShouldUpdate( Tr2Lod lod, float timeSinceUpdate )
	{
		switch( lod )
		{
		case TR2_LOD_UNSPECIFIED:
		case TR2_LOD_LOW:
			return timeSinceUpdate >= g_eveSpaceSceneLowUpdateRate;
			break;
		case TR2_LOD_MEDIUM:
			return timeSinceUpdate >= g_eveSpaceSceneMediumUpdateRate;
			break;
		default:
			break;
		}

		return true;
	}

	// --------------------------------------------------------------------------------
	// Description:
	//   Examines the two LODs and returns the higher(valid) of the two
	// Arguments:
	//   lod0 - first LOD to examine
	//   lod1 - second LOD to examine
	// Return value:
	//   Returns the higher(valid) LOD of the two LODs passed in, or TR2_LOD_UNSPECIFIED if
	//   neither LOD is valid.
	// --------------------------------------------------------------------------------
	static inline Tr2Lod MergeLOD( Tr2Lod lod0, Tr2Lod lod1 )
	{
		if( lod0 == TR2_LOD_UNSPECIFIED || lod1 == TR2_LOD_UNSPECIFIED )
		{
			return ( lod0 == TR2_LOD_UNSPECIFIED ) ? lod1 : lod0;
		}
		return ( lod0 > lod1 ) ? lod0 : lod1;
	}

	// --------------------------------------------------------------------------------
	// Description:
	//   Calculates the lod based on the sphere and the frustum and then merges the
	//   results
	// Arguments:
	//   lod0 - first LOD to examine
	//   sphere - sphere used to calculate second LOD
	//   frustum - frustum used to calculate second LOD
	// Return value:
	//   Returns the higher(valid) LOD
	// --------------------------------------------------------------------------------
	static Tr2Lod MergeLOD( Tr2Lod lod0, const Vector4& sphere, const EveUpdateContext& updateContext );
};


#endif