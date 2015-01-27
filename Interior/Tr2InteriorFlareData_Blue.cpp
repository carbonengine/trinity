#include "StdAfx.h"

#include "Tr2InteriorFlareData.h"

BLUE_DEFINE( Tr2InteriorFlareData );

const Be::ClassInfo* Tr2InteriorFlareData::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorFlareData, "" )
        MAP_INTERFACE( Tr2InteriorFlareData )

		MAP_ATTRIBUTE( 
			"positionWeight", 
			m_positionWeight, 
			"Proportion of vector from screen center to "
			"flare position on the screen where this particular "
			"flare image appears (could be negative). For example "
			"(1, 1) would be flare position, (0, 0) - screen center.", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"size", 
			m_size, 
			"Flare size (in post-projection space)", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"textureOffset", 
			m_textureOffset, 
			"Offset of the flare image in texture atlas (from 0 to 1)", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"textureSize", 
			m_textureSize, 
			"Size of the flare image in texture atlas (from 0 to 1)", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"rotation", 
			m_rotation, 
			"Billboard rotation (rotation dependant on screen position)", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"directionalStretch", 
			m_directionalStretch, 
			"Directional stretch (dependant on screen position)", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"edgeFadeDistance", 
			m_edgeFadeDistance, 
			"Fade out at screen edges distance", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"centerFadeMinRadius", 
			m_centerFadeMinRadius, 
			"Fade out in the center of the screen min radius", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"centerFadeMaxRadius", 
			m_centerFadeMaxRadius, 
			"Fade out in the center of the screen max radius", 
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}

