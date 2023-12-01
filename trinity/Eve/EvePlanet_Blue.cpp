#include "StdAfx.h"
#include "EvePlanet.h"
#include "EveTransform.h"

BLUE_DEFINE( EvePlanet );

const Be::ClassInfo* EvePlanet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EvePlanet, "" )
		MAP_INTERFACE( EvePlanet )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2SecondaryLightSource )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( IWorldPosition )
		MAP_ATTRIBUTE
		(
			"radius",
			m_radius,
			"The planet's radius",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"albedoColor",
			m_albedoColor,
			"Planet albedo color, used for secondary lighting",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"minScreenSize",
			m_minScreenSize,
			"Minimum screen size for planet rendering",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"zOnlyModel",
			m_zOnlyModel,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"emissiveColor",
			m_emissiveColor,
			"Color of the secondary light source",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"estimatedPixelDiameter",
			m_estimatedPixelDiameter,
			"Planet size in pixels as it appears from the camera perspective. ",
			Be::READ | Be::PERSIST )
	EXPOSURE_CHAINTO(EveEffectRoot2)
}
