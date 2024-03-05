////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef ITr2GenericParticleConstraint_H
#define ITr2GenericParticleConstraint_H

BLUE_DECLARE( Tr2ParticleSystem );

#include "ITr2GenericEmitter.h"

// --------------------------------------------------------------------------------------
// Description:
//   ITr2GenericParticleConstraint is a particle constraint type for Tr2ParticleSystem.
//   Particle constaints are arbitrary operations that can be applied to particles (for
//   example collisions).
// See Also:
//   Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2GenericParticleConstraint ) : public IRoot
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Generates data for new particle component (element). This method can be called
	//	 asyncronously.
	// Arguments:
	//   arguments - arguments for child emitters
	//   paticles - Particle data stream: Tr2ParticleElementData::COUNT of float arrays. 
	//		The constraint can modify any data element of a particle.
	//   strides - Sizes of particle data in each of "particles" arrays (in floats).
	//   count - Number of particles.
	//   dt - Frame time.
	// ----------------------------------------------------------------------------------
	virtual void ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particles, unsigned* strides, unsigned count, float dt ) = 0;

	// ----------------------------------------------------------------------------------
	// Description:
	//   A chance for constraint to bind itself to a particle system. Called when 
	//   constaint is added to the particle system or when system is re-binded.
	// Arguments:
	//   system - Particle system the constaint is attached to.
	// ----------------------------------------------------------------------------------
	virtual void Bind( Tr2ParticleSystem* system ) = 0;
};

BLUE_DECLARE_IVECTOR( ITr2GenericParticleConstraint );

#endif // ITr2GenericParticleConstraint_H