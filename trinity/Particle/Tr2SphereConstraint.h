////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SphereConstraint_H
#define Tr2SphereConstraint_H

#include "ITr2GenericParticleConstraint.h"
#include "Tr2ParticleElementDeclaration.h"

BLUE_DECLARE_INTERFACE( ITr2AttributeGenerator );
BLUE_DECLARE_IVECTOR( ITr2AttributeGenerator );
BLUE_DECLARE_INTERFACE( ITr2GenericEmitter );
BLUE_DECLARE_IVECTOR( ITr2GenericEmitter );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2SphereConstraint is a constraint type for Tr2ParticleSystem that models particle
//   collision with a sphere. When collision is detected it can apply physics collision
//   response to a particle and/or generate new values for particle data components.
// See Also:
//   Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2SphereConstraint ): 
	public ITr2GenericParticleConstraint,
	public IInitialize,
	public IListNotify
{
public:
	Tr2SphereConstraint( IRoot* lockobj = 0 );

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////
	// INotify
	virtual void OnListModified(
		long event,		// BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList
		);

	/////////////////////////////////////////////////////////////
	// ITr2ParticleConstraint
	virtual void ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particle, unsigned* strides, unsigned count, float dt );
	virtual void Bind( Tr2ParticleSystem* system );

private:
	// Collision sphere center
	Vector3 m_position;
	// Collision sphere radius
	float m_radius;
	// Invert sphere test
	bool m_invertSphere;
	// Inverse friction coefficient
	float m_friction;
	// Inverse elasticity coefficient
	float m_elasticity;
	// Reflection noise coefficient
	float m_reflectionNoise;
	// Name of particle component that stores radius
	std::string m_particleRadiusComponent;
	// Multiplier for particle radius component data to get the actual radius
	Vector4 m_particleRadiusCoefficient;
	// Apply collision response to particle positions
	bool m_affectPosition;
	// Apply collision response to particle velocities
	bool m_affectVelocity;
	// List of particle element generators
	PITr2AttributeGeneratorVector m_generators;
	// Was the emitter successfully bound to the particle system
	bool m_isValid;
	// Particle element data for position
	Tr2ParticleElementData m_positionElement;
	// Particle element data for velocity
	Tr2ParticleElementData m_velocityElement;
	// Particle element data for particle radius
	Tr2ParticleElementData m_radiusElement;
	// On collision emitters
	PITr2GenericEmitterVector m_onCollisionEmitters;
};

TYPEDEF_BLUECLASS( Tr2SphereConstraint );

#endif // Tr2SphereConstraint_H