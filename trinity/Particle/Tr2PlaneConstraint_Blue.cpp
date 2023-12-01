////////////////////////////////////////////////////////////
//
//    Created:   February 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2PlaneConstraint.h"

BLUE_DEFINE( Tr2PlaneConstraint );

const Be::ClassInfo* Tr2PlaneConstraint::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2PlaneConstraint, "" )
		MAP_INTERFACE( ITr2GenericParticleConstraint )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( Tr2PlaneConstraint )

		MAP_ATTRIBUTE( "plane",  m_plane,  "Collision plane", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "affectPosition",  m_affectPosition,  "Apply collision response to particle positions", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "affectVelocity",  m_affectVelocity,  "Apply collision response to particle velocities", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "friction",  m_friction,  "Inverse friction coefficient", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "elasticity",  m_elasticity,  "Inverse elasticity coefficient", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reflectionNoise",  m_reflectionNoise,  "Reflection noise coefficient", Be::READWRITE | Be::PERSIST )
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
