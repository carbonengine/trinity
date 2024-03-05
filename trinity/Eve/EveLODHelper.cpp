#include "StdAfx.h"
#include "EveLODHelper.h"

#include "TriFrustum.h"

// externs
extern float g_eveSpaceSceneLowDetailThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;


// --------------------------------------------------------------------------------
// Description:
//   First calculate the LOD based on the provided sphere and frustum, then "merge"
//   it with lod0
// --------------------------------------------------------------------------------
Tr2Lod EveLODHelper::MergeLOD( Tr2Lod lod0, const Vector4& sphere, const TriFrustum& frustum )
{
	// only change something if this thing is actually visible
	if( frustum.IsSphereVisible( &sphere ) )
	{
		Tr2Lod lod1 = TR2_LOD_LOW;

		float estimatedSize = frustum.GetPixelSizeAccross( &sphere );
		if( estimatedSize >= g_eveSpaceSceneMediumDetailThreshold )
		{
			lod1 = TR2_LOD_HIGH;
		}
		else if( estimatedSize >= g_eveSpaceSceneLowDetailThreshold )
		{
			lod1 = TR2_LOD_MEDIUM;
		}

		// ok, merge it!
		return MergeLOD( lod0, lod1 );
	}
	return TR2_LOD_UNSPECIFIED;
}



