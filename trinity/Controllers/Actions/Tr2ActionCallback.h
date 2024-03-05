////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#pragma once

#include "ITr2ControllerAction.h"



BLUE_CLASS( Tr2ActionCallback ) : public ITr2ControllerAction
{
public:
	EXPOSE_TO_BLUE();

	virtual void Start( Tr2Controller& controller );
private:
	BlueSharedString m_callbackName;
};

TYPEDEF_BLUECLASS( Tr2ActionCallback );
