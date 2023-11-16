////////////////////////////////////////////////////////////
//
//    Created:   March 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2TextureAnimationParameter.h"


BLUE_DEFINE( Tr2TextureAnimationParameter );

const Be::ClassInfo* Tr2TextureAnimationParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TextureAnimationParameter, "" )

		MAP_INTERFACE( Tr2TextureAnimationParameter )
		MAP_INTERFACE( ITriEffectResourceParameter )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Parameter name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "channel", m_channel, "Channel name for multi-channel animations", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "animation", m_animation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
