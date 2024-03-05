////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2ControllerAction.h"

BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );


BLUE_CLASS( Tr2ActionChildEffect ) : public ITr2ControllerAction
{
public:
	Tr2ActionChildEffect( IRoot* lockobj = nullptr );
	EXPOSE_TO_BLUE();

	virtual void Start( Tr2Controller& controller );
	virtual void Stop( Tr2Controller& controller );
	virtual void Link( Tr2Controller& controller );
private:
	std::string m_path;
	std::string m_childName;
	IEveSpaceObjectChildPtr m_child;
	bool m_addOnStart;
	bool m_removeOnStop;
	BlueSharedString m_targetAnotherOwner;
};

TYPEDEF_BLUECLASS( Tr2ActionChildEffect );
