// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveSpriteSet.h"
#include "Shader/Tr2Effect.h"

BLUE_DEFINE( EveSpriteSet );

const Be::ClassInfo* EveSpriteSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpriteSet, "" )
		MAP_INTERFACE( EveSpriteSet )
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
			"sprites",
			m_sprites,
			"",
			Be::READ | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"Effect to use for rendering sprites",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"display",
			m_display,
			"Specifies whether to render the object or not",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"skinned",
			m_skinned,
			"Is the sprite set skinned (requires that the owner object is skinned)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"intensity",
			m_intensity,
			"Overall sprite intensity",
			Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild resources after adding/removing/changing individual sprites" )

	EXPOSURE_END()
}
