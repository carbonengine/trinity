// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionCallback.h"
#include "Controllers/Tr2Controller.h"

void Tr2ActionCallback::Start( ITr2ActionController& controller )
{
	if( !m_callbackName.empty() )
	{
		controller.Callback( m_callbackName );
	}
}
