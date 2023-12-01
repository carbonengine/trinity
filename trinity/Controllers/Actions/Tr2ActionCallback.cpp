////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2ActionCallback.h"
#include "Controllers/Tr2Controller.h"

void Tr2ActionCallback::Start( Tr2Controller& controller )
{
	if( !m_callbackName.empty() )
	{
		controller.Callback( m_callbackName );
	}
}
