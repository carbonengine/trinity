#include "StdAfx.h"
#include "EveBoosterSet2.h"

BLUE_DEFINE( EveBoosterSet2 );

const Be::ClassInfo* EveBoosterSet2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveBoosterSet2, "" )
        MAP_INTERFACE( EveBoosterSet2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
        MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE
		(
			"display", 
			m_display, 
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE( "volumetric", m_isVolumetric, "booster uses volumetric shader", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "physicsUpdate", m_physicsUpdate, "This enables updating of the boosters trails based on physics sim", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "destinyUpdate", m_destinyUpdate, "This enables updating speed etc. from destiny simulation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "drawDebugInfo", m_drawDebugInfo, "Enable debug drawing", Be::READWRITE )
		MAP_ATTRIBUTE( "boosterLOD", m_boosterLOD, "current booster LOD", Be::READ )
		MAP_ATTRIBUTE( "trailsLOD", m_trailsLOD, "current trails LOD", Be::READ )
		MAP_ATTRIBUTE
		(
			"effect",  
			m_effect, 
			"Effect to use to render the boosters",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"glows",  
			m_glows, 
			"Sprite set to use to render the glows on the boosters",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE( "trails", m_trails, "Trails set used to render the trails of this booster", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "overallIntensity", m_overallIntensity, "The overall intensity of the boosters resulting from velo, acceleration, etc.", Be::READ )
		MAP_ATTRIBUTE( "maxSize", m_maxSize, "The biggest booster size of this set.", Be::READ )
		MAP_ATTRIBUTE( "parentSpeed", m_parentSpeed, "The speed of the ship", Be::READWRITE )
		MAP_ATTRIBUTE( "warpIntensity", m_warpIntensity, "The warp factor of the ship", Be::READWRITE )
		MAP_ATTRIBUTE( "parentRotation", m_parentRotation, "The rotation of the ship", Be::READWRITE )
#if BLUE_WITH_PYTHON
		MAPFLOATARRAYSIZE( "boosterBoundingSphereCenter", m_boosterBoundingSphere, BlueDefaultIID, "The center of the minimum bounding sphere of the boosters", Be::READ, 3 )
#endif

		MAP_ATTRIBUTE( "boosterBoundingSphereRadius", m_boosterBoundingSphere.w, "The radius of the minimum bounding sphere of the boosters", Be::READ )
		MAP_ATTRIBUTE( "trailsBoundsMin", m_trailsBoundsMin, "Minimum aa bounding box of the trails", Be::READ )
		MAP_ATTRIBUTE( "trailsBoundsMax", m_trailsBoundsMax, "Maximum aa bounding box of the trails", Be::READ )

		MAP_ATTRIBUTE
		(
			"glowScale",  
			m_glowScale, 
			"Scale of glow sprites",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"glowColor",       
			m_glowColor,       
			"Color of glow sprites", 
			Be::READWRITE | Be::PERSIST
		)	
		MAP_ATTRIBUTE
		(
			"symHaloScale",  
			m_symHaloScale, 
			"Scale on halo sprites",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"haloScaleX",  
			m_haloScaleX, 
			"Scale on halo sprites",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"haloScaleY",
			m_haloScaleY,
			"Scale on halo sprites",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"maxVel",
			m_maxVel,
			"Max velocity of the ball",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(
			"alwaysOn",
			m_alwaysOn,
			"Toggle whether the boosters are always on or not. Useful for Jessica and marketing reasons.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"alwaysOnIntensity",
			m_alwaysOnIntensity,
			"Booster intensity when alwaysOn is turned on. Useful for Jessica and marketing reasons.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"haloColor",       
			m_haloColor,       
			"Color of glow sprites", 
			Be::READWRITE | Be::PERSIST
		)	

		MAP_ATTRIBUTE( "trailsSmoothing", m_trailsSmoothing, "Smoothness for bending (length of tangents of splines)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "trailsTimeDelta", m_trailsTimeDelta, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "trailsStaticOffsets0", m_trailsStaticOffsets[0], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets1", m_trailsStaticOffsets[1], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets2", m_trailsStaticOffsets[2], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets3", m_trailsStaticOffsets[3], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets4", m_trailsStaticOffsets[4], "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "trailIntensity", m_trailIntensity, "", Be::READ )
		MAP_ATTRIBUTE( "trailsTotalLength", m_trailsTotalLength, "", Be::READ )

	EXPOSURE_END()
}
