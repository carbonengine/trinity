// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "EveSpotlightSet.h"

BLUE_DEFINE( EveSpotlightSet );

const Be::ClassInfo* EveSpotlightSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpotlightSet, "" )
		MAP_INTERFACE( EveSpotlightSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IEveSpaceObjectAttachment )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"display",
			m_display,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"skinned",
			m_skinned,
			"Is the set skinned (requires that the owner object to be skinned)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"intensity",
			m_intensity,
			"Overall intensity",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"coneEffect",
			m_coneEffect,
			"Effect to use to render the spotlight cones",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"glowEffect",
			m_glowEffect,
			"Effect to use to render the spotlight glows",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"spotlightItems",
			m_spotlightItems,
			"",
			Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild resources after adding/removing/changing individual items" )


	EXPOSURE_END()
}
