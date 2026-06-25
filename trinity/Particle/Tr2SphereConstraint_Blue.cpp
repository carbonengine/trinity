// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2SphereConstraint.h"

BLUE_DEFINE( Tr2SphereConstraint );

const Be::ClassInfo* Tr2SphereConstraint::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SphereConstraint, "" )
		MAP_INTERFACE( ITr2GenericParticleConstraint )
		MAP_INTERFACE( Tr2SphereConstraint )

		MAP_ATTRIBUTE( "position", m_position, "Collision sphere center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radius", m_radius, "Collision sphere radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "invertSphere", m_invertSphere, "Invert sphere test", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "affectPosition", m_affectPosition, "Apply collision response to particle positions", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "affectVelocity", m_affectVelocity, "Apply collision response to particle velocities", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "friction", m_friction, "Inverse friction coefficient", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "elasticity", m_elasticity, "Inverse elasticity coefficient", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reflectionNoise", m_reflectionNoise, "Reflection noise coefficient", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"particleRadiusComponent",
			m_particleRadiusComponent,
			"Name of the particle component that contains particle size (leave blank to have size=0 for all particles)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"particleRadiusCoefficient",
			m_particleRadiusCoefficient,
			"What to treat as particle surface (0 - center, 1 - sphere with particle radius)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "generators", m_generators, "List of generators that are executed on collision", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "onCollisionEmitters", m_onCollisionEmitters, "List of emitters that are executed on collision", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "isValid", m_isValid, "Is the constraint successfully bound to the particle system", Be::READ )

	EXPOSURE_END()
}
