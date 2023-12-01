#pragma once
#ifndef EveParticleDragForce_H
#define EveParticleDragForce_H

#include "Particle/Tr2ParticleDragForce.h"

BLUE_DECLARE( EveParticleDragForce );

// --------------------------------------------------------------------------------------
// Description:
//   Drag force for particle systems. All the code was moved to Tr2ParticleDragForce. 
//   This class stays for backward compatibility.
// See Also:
//   Tr2ParticleDragForce
// --------------------------------------------------------------------------------------
class EveParticleDragForce:
	public Tr2ParticleDragForce
{
public:
	EXPOSE_TO_BLUE();

	EveParticleDragForce( IRoot* lockobj = 0 );
};

TYPEDEF_BLUECLASS( EveParticleDragForce );
BLUE_DECLARE_VECTOR( EveParticleDragForce );

#endif