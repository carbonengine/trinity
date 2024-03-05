#pragma once
#ifndef EveParticleSpringAttractor_H
#define EveParticleSpringAttractor_H

#include "Particle/Tr2ParticleSpring.h"

BLUE_DECLARE( EveParticleSpring );

// --------------------------------------------------------------------------------------
// Description:
//   Spring force for particle systems. All the code was moved to Tr2ParticleSpring. 
//   This class stays for backward compatibility.
// See Also:
//   Tr2ParticleSpring
// --------------------------------------------------------------------------------------
class EveParticleSpring:
	public Tr2ParticleSpring
{
public:
	EXPOSE_TO_BLUE();

	EveParticleSpring( IRoot* lockobj = 0 );;
};

TYPEDEF_BLUECLASS( EveParticleSpring );
BLUE_DECLARE_VECTOR( EveParticleSpring );

#endif