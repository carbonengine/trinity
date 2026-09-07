// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildBoosterSet.h"


BLUE_DEFINE( EveChildBoosterSet );

const Be::ClassInfo* EveChildBoosterSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildBoosterSet, "" )
		MAP_INTERFACE( EveChildBoosterSet )
		MAP_INTERFACE( EveSpaceObjectChild )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( ITr2ControllerOwner )

		MAP_ATTRIBUTE( "name", m_name, "Name of the space object child", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"display",
			m_display,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxSize", m_maxSize, "The biggest booster size of this set.", Be::READ )
		MAP_ATTRIBUTE( "warpIntensity", m_warpIntensity, "The warp factor of the ship.", Be::READWRITE )
		MAP_ATTRIBUTE( "thrust", m_thrust, "The thrust of the ship.", Be::READWRITE )
#if BLUE_WITH_PYTHON
		MAPFLOATARRAYSIZE( "boosterBoundingSphereCenter", m_boosterBoundingSphere, BlueDefaultIID, "The center of the minimum bounding sphere of the boosters", Be::READ, 3 )
#endif

		MAP_ATTRIBUTE( "boosterBoundingSphereRadius", m_boosterBoundingSphere.w, "The radius of the minimum bounding sphere of the boosters", Be::READ )

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
			"driveName",
			m_driveName,
			"Controller name the booster observes for thrust value.\n",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "flareLodEnabled", m_flareLodEnabled, ":jessica-group: Fakery", Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE( "effect", m_effect, "Effect to use to render the boosters", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "effectFar", m_effectFar, "Effect to use to render the boosters at a distance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glows", m_glows, "Sprite set to use to render the glows on the boosters", Be::READWRITE | Be::PERSIST )


	EXPOSURE_END()
}
