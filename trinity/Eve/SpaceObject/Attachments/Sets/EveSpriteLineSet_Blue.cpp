////////////////////////////////////////////////////////////
//
//    Created:   January 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"
#include "EveSpriteLineSet.h"

BLUE_DEFINE( EveSpriteLineSet );

const Be::ClassInfo* EveSpriteLineSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSpriteLineSet, "" )
        MAP_INTERFACE( EveSpriteLineSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IEveSpaceObjectAttachment )

		MAP_ATTRIBUTE( "display", m_display, "Specifies whether to render this set or not", Be::READWRITE )
		MAP_ATTRIBUTE( "name", m_name, "Standard name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "Follows bone animation?", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "effect", m_effect, "Shader for blinkies", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "effectHash", m_effectHash, "Shader for blinkies", Be::READ )

		MAP_ATTRIBUTE( "spriteLines", m_spriteLines, "All the line items", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild resources after adding/removing/changing individual sprites" )

	EXPOSURE_END()
}
