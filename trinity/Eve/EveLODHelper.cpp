// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveLODHelper.h"


// --------------------------------------------------------------------------------
// Description:
//   First calculate the LOD based on the provided sphere and frustum, then "merge"
//   it with lod0
// --------------------------------------------------------------------------------
Tr2Lod EveLODHelper::MergeLOD( Tr2Lod lod0, const Vector4& sphere, const EveUpdateContext& updateContext )
{
	auto& frustum = updateContext.GetFrustum();

	// only change something if this thing is actually visible
	if( frustum.IsSphereVisible( &sphere ) )
	{
		Tr2Lod lod1 = TR2_LOD_LOW;

		float estimatedSize = frustum.GetPixelSizeAccross( &sphere );
		if( estimatedSize >= updateContext.GetMediumDetailThreshold() )
		{
			lod1 = TR2_LOD_HIGH;
		}
		else if( estimatedSize >= updateContext.GetLowDetailThreshold() )
		{
			lod1 = TR2_LOD_MEDIUM;
		}

		// ok, merge it!
		return MergeLOD( lod0, lod1 );
	}
	return TR2_LOD_UNSPECIFIED;
}
