// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveParticleDirectForce_H
#define EveParticleDirectForce_H

#include "Particle/Tr2ParticleDirectForce.h"

BLUE_DECLARE( EveParticleDirectForce );

// --------------------------------------------------------------------------------------
// Description:
//   Constant force for particle systems. All the code was moved to Tr2ParticleDirectForce.
//   This class stays for backward compatibility.
// See Also:
//   Tr2ParticleDirectForce
// --------------------------------------------------------------------------------------
class EveParticleDirectForce : public Tr2ParticleDirectForce
{
public:
	EXPOSE_TO_BLUE();

	EveParticleDirectForce( IRoot* lockobj = 0 );
};

TYPEDEF_BLUECLASS( EveParticleDirectForce );
BLUE_DECLARE_VECTOR( EveParticleDirectForce );

#endif