////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2ActionCallback.h"


BLUE_DEFINE( Tr2ActionCallback );

const Be::ClassInfo* Tr2ActionCallback::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionCallback, "" )
		MAP_INTERFACE( Tr2ActionCallback )
		MAP_INTERFACE( ITr2ControllerAction )

		MAP_ATTRIBUTE(
			"callbackName",
			m_callbackName,
			"The name of the callback that is registered on the controller",
			Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}
