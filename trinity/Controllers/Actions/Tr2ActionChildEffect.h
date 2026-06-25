// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"

BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );


BLUE_CLASS( Tr2ActionChildEffect ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionChildEffect( IRoot* lockobj = nullptr );
	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;
	void Stop( ITr2ActionController & controller ) override;
	void Link( ITr2ActionController & controller ) override;

private:
	std::string m_path;
	std::string m_childName;
	IEveSpaceObjectChildPtr m_child;
	bool m_addOnStart;
	bool m_removeOnStop;
	BlueSharedString m_targetAnotherOwner;
};

TYPEDEF_BLUECLASS( Tr2ActionChildEffect );
