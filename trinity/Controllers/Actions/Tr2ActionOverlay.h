////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2ControllerAction.h"


BLUE_DECLARE( EveMeshOverlayEffect );


BLUE_CLASS( Tr2ActionOverlay ) : public ITr2ControllerAction
{
public:
    Tr2ActionOverlay( IRoot* lockobj = nullptr );
	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController& controller ) override;
	void Stop( ITr2ActionController& controller ) override;

private:
	std::string m_path;
	EveMeshOverlayEffectPtr m_overlay;
	BlueSharedString m_targetAnotherOwner;
    bool m_addOnStart;
    bool m_removeOnStop;
};

TYPEDEF_BLUECLASS( Tr2ActionOverlay );
