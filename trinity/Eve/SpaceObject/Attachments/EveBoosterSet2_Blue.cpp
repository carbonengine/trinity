// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveBoosterSet2.h"

BLUE_DEFINE( EveBoosterSet2Renderable );

const Be::ClassInfo* EveBoosterSet2Renderable::ExposeToBlue(){
	EXPOSURE_BEGIN( EveBoosterSet2Renderable, "" )
		MAP_INTERFACE( EveBoosterSet2Renderable )
			MAP_INTERFACE( ITr2Renderable )

				MAP_ATTRIBUTE( "overallIntensity", m_overallIntensity, "The overall intensity of the boosters resulting from velo, acceleration, etc.", Be::READ )
					MAP_ATTRIBUTE( "parentSpeed", m_parentSpeed, "The speed of the ship", Be::READWRITE )
						MAP_ATTRIBUTE( "parentRotation", m_parentRotation, "The rotation of the ship", Be::READWRITE )
							MAP_ATTRIBUTE( "trailsBoundsMin", m_trailsBoundsMin, "Minimum aa bounding box of the trails", Be::READ )
								MAP_ATTRIBUTE( "trailsBoundsMax", m_trailsBoundsMax, "Maximum aa bounding box of the trails", Be::READ )
									MAP_ATTRIBUTE( "trailsTimeDelta", m_trailsTimeDelta, "", Be::READWRITE | Be::PERSIST )
										MAP_ATTRIBUTE( "trailIntensity", m_trailIntensity, "", Be::READ )
											MAP_ATTRIBUTE( "trailsTotalLength", m_trailsTotalLength, "", Be::READ )
												MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READ )
													MAP_ATTRIBUTE( "trailsVisible", m_trailsVisible, "", Be::READ )
														MAP_ATTRIBUTE( "boostersVisible", m_boostersVisible, "", Be::READ )
															MAP_ATTRIBUTE( "boosterHighLod", m_boosterHighLod, "Are the boosters using high lods (using the near effect)", Be::READ )
																EXPOSURE_END()
}


BLUE_DEFINE( EveBoosterSet2 );

const Be::ClassInfo* EveBoosterSet2::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveBoosterSet2, "" )
		MAP_INTERFACE( EveBoosterSet2 )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE(
			"display",
			m_display,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "physicsUpdate", m_physicsUpdate, "This enables updating of the boosters trails based on physics sim", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "destinyUpdate", m_destinyUpdate, "This enables updating speed etc. from destiny simulation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxSize", m_maxSize, "The biggest booster size of this set.", Be::READ )
		MAP_ATTRIBUTE( "warpIntensity", m_warpIntensity, "The warp factor of the ship", Be::READWRITE )
#if BLUE_WITH_PYTHON
		MAPFLOATARRAYSIZE( "boosterBoundingSphereCenter", m_boosterBoundingSphere, BlueDefaultIID, "The center of the minimum bounding sphere of the boosters", Be::READ, 3 )
#endif

		MAP_ATTRIBUTE( "boosterBoundingSphereRadius", m_boosterBoundingSphere.w, "The radius of the minimum bounding sphere of the boosters", Be::READ )

		MAP_ATTRIBUTE(
			"maxVel",
			m_maxVel,
			"Max velocity of the ball",
			Be::READWRITE )

		MAP_ATTRIBUTE( "trailsSmoothing", m_trailsSmoothing, "Smoothness for bending (length of tangents of splines)", Be::READWRITE | Be::PERSIST )

		// glows
		MAP_ATTRIBUTE(
			"glowScale",
			m_glowScale,
			"Scale of glow sprites\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"glowColor",
			m_glowColor,
			"Color of glow sprites\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"symHaloScale",
			m_symHaloScale,
			"Scale on halo sprites\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"haloScaleX",
			m_haloScaleX,
			"Scale on halo sprites\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"haloScaleY",
			m_haloScaleY,
			"Scale on halo sprites\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"haloColor",
			m_haloColor,
			"Color of glow sprites\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"warpGlowColor",
			m_warpGlowColor,
			"Color of glow sprites in warp\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"warpHaloColor",
			m_warpHaloColor,
			"Color of halo sprites in warp\n"
			":jessica-group: Glow",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "lightOffset", m_lightOffset, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightFlickerAmplitude", m_lightFlickerAmplitude, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightFlickerFrequency", m_lightFlickerFrequency, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightRadius", m_lightRadius, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightColor", m_lightColor, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightWarpRadius", m_lightWarpRadius, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightWarpColor", m_lightWarpColor, ":jessica-group: Lights", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"alwaysOn",
			m_alwaysOn,
			"Toggle whether the boosters are always on or not. Useful for Jessica and marketing reasons.\n"
			":jessica-group: Fakery",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"alwaysOnIntensity",
			m_alwaysOnIntensity,
			"Booster intensity when alwaysOn is turned on. Useful for Jessica and marketing reasons.\n"
			":jessica-group: Fakery",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "staticTrailLength", m_staticTrailLength, ":jessica-group: Fakery", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "flareLodEnabled", m_flareLodEnabled, ":jessica-group: Fakery", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "trailsStaticOffsets0", m_trailsStaticOffsets[0], ":jessica-group: Fakery", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets1", m_trailsStaticOffsets[1], ":jessica-group: Fakery", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets2", m_trailsStaticOffsets[2], ":jessica-group: Fakery", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets3", m_trailsStaticOffsets[3], ":jessica-group: Fakery", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailsStaticOffsets4", m_trailsStaticOffsets[4], ":jessica-group: Fakery", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "instances", m_boosterRenderables, "Instances of this booster set.", Be::READ )
		MAP_ATTRIBUTE( "effect", m_effect, "Effect to use to render the boosters", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "effectFar", m_effectFar, "Effect to use to render the boosters at a distance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glows", m_glows, "Sprite set to use to render the glows on the boosters", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trails", m_trails, "Trails set used to render the trails of this booster", Be::READWRITE | Be::PERSIST )


	EXPOSURE_END()
}
