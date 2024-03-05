////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2ControllerAction.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"


BLUE_CLASS( Tr2ActionResetClipSphereCenter ) : public ITr2ControllerAction
{
public:
	enum ResetBehavior
	{
		OBJECT_CENTER,
		LAST_DAMAGELOCATOR_HIT,
		CUSTOM,
	};

	Tr2ActionResetClipSphereCenter( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void Start( Tr2Controller& controller );

private:
	void ResetClipSphereToLocator( EveSpaceObject2* owner, BlueSharedString locatorSetName, int locatorIndex );

	BlueSharedString m_locatorSetName;
	int32_t m_locatorIndex;
	ResetBehavior m_resetBehavior;
};

TYPEDEF_BLUECLASS( Tr2ActionResetClipSphereCenter );
