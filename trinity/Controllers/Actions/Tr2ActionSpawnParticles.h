// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"

BLUE_DECLARE( Tr2DynamicEmitter );


BLUE_CLASS( Tr2ActionSpawnParticles ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionSpawnParticles( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;

private:
	Tr2DynamicEmitterPtr m_emitter;
	float m_rate;
};

TYPEDEF_BLUECLASS( Tr2ActionSpawnParticles );
