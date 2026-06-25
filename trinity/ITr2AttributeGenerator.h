// Copyright © 2011 CCP ehf.

#pragma once
#ifndef ITr2AttributeGenerator_H
#define ITr2AttributeGenerator_H

#include "Particle/Tr2ParticleElementDeclaration.h"

// --------------------------------------------------------------------------------------
// Description:
//   ITr2AttributeGenerator is an interface for particle system attribute generators.
//   Used by particle emitters to generate each new particle component.
// See Also:
//   Tr2DynamicEmitter, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2AttributeGenerator ) :
	public IRoot
{
	// --------------------------------------------------------------------------------------
	// Description:
	//   Generates data for new particle component (element).
	// Arguments:
	//   position - Position of the "parent" particle (if the emitter owning this generator
	//		is "emit during life" or "emit on death" emitter and parent particle has
	//		position element); otherwise is nullptr.
	//   velocity - Velocity of the "parent" particle (if the emitter owning this generator
	//		is "emit during life" or "emit on death" emitter and parent particle has
	//		velocity element); otherwise is nullptr.
	//   paticle - (out) New particle data: Tr2ParticleElementData::COUNT of float arrays.
	//		The generator is responsible for filling only portions of this data where
	//		particle elements it is assigned to reside.
	// --------------------------------------------------------------------------------------
	virtual void Generate( const Vector3* position, const Vector3* velocity, float** particle ) = 0;

	// --------------------------------------------------------------------------------------
	// Description:
	//   "Binds" generator to a particle system. This is a chance for generator to find
	//   buffer types, offsets, etc. of particle elements it is supposed to generate.
	// Arguments:
	//   declaration - Particle element data coming from particle system.
	//   boundElements - (in/out) The generator is expected to mark particle elements it will
	//		be filling by adding their declaration names to this set. Emitter uses this set
	//      to check if all particle elements were bound to some generator. The generator
	//		is responsible for checking if its elements are overwritten by some other
	//		generator using this set.
	// Return Value:
	//   true If the generator successfully binds to the particle system
	//   false Otherwise
	// --------------------------------------------------------------------------------------
	virtual bool Bind( const Tr2ParticleElementDataMap& declaration,
					   std::set<Tr2ParticleElementDeclarationName>& boundElements ) = 0;
};

BLUE_DECLARE_IVECTOR( ITr2AttributeGenerator );

#endif // ITr2AttributeGenerator_H