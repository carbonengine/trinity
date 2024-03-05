////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "EveBannerSet.h"

BLUE_DEFINE( EveBannerSet );

const Be::ClassInfo* EveBannerSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveBannerSet, "" )
		MAP_INTERFACE( EveBannerSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IEveSpaceObjectAttachment )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "key", m_key, "Banner contents type", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "effect", m_effect, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE )
		MAP_ATTRIBUTE( "isPickable", m_isPickable, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "banners", m_banners, "", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild internal buffers" )
		MAP_METHOD_AND_WRAP( 
			"GetReference", 
			GetReference, 
			"Returns private reference ID stored for each banner\n"
			":param index: banner index\n"
		)
	EXPOSURE_END()
}