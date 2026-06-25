// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"


BLUE_DECLARE( EveMeshOverlayEffect );


BLUE_CLASS( Tr2ActionOverlay ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionOverlay( IRoot* lockobj = nullptr );
	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;
	void Stop( ITr2ActionController & controller ) override;

private:
	void LoadOverlay( EveSpaceObject2 * owner );

	std::string m_path;
	std::string m_overlayName;
	EveMeshOverlayEffectPtr m_overlay;
	BlueSharedString m_targetAnotherOwner;
	bool m_addOnStart;
	bool m_removeOnStop;
};

TYPEDEF_BLUECLASS( Tr2ActionOverlay );
