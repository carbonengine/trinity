////////////////////////////////////////////////////////////
//
//    Created:   September 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2ControllerAction.h"

BLUE_DECLARE( Tr2DynamicEmitter );


BLUE_CLASS( Tr2ActionSpawnParticles ) : public ITr2ControllerAction
{
public:
	Tr2ActionSpawnParticles( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void Start( Tr2Controller& controller );
private:
	Tr2DynamicEmitterPtr m_emitter;
	float m_rate;
};

TYPEDEF_BLUECLASS( Tr2ActionSpawnParticles );
