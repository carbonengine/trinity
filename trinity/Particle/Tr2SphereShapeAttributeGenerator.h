// Copyright © 2011 CCP ehf.

#pragma once
#ifndef Tr2SphereShapeAttributeGenerator_H
#define Tr2SphereShapeAttributeGenerator_H

#include "ITr2AttributeGenerator.h"

// --------------------------------------------------------------------------------------
// Description:
//   Particle element generator that generates particle positions and/or velocities as if
//   particles are spawned in a sphere sector (with velocities pointing from the sphere
//   center).
// See Also:
//   ITr2AttributeGenerator, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2SphereShapeAttributeGenerator ) :
	public ITr2AttributeGenerator
{
public:
	EXPOSE_TO_BLUE();

	Tr2SphereShapeAttributeGenerator();
	~Tr2SphereShapeAttributeGenerator();

	void Generate( const Vector3* position, const Vector3* velocity, float** particle );
	bool Bind( const Tr2ParticleElementDataMap& declaration,
			   std::set<Tr2ParticleElementDeclarationName>& boundElements );
	std::string GetName() const;

	void GetTransform( Vector3 & position, Quaternion & rotation );
	void SetTransform( const Vector3& position, const Quaternion& rotation );

private:
	// Generator position
	Vector3 m_position;
	// Generator rotation
	Quaternion m_rotation;

	// Angles for emitting cones
	float m_minPhi, m_maxPhi;
	float m_minTheta, m_maxTheta;
	float m_minRadius, m_maxRadius;

	float m_distributionExponent;

	// Does the generator generate positions
	bool m_controlPosition;
	// Does the generator generate velocities
	bool m_controlVelocity;
	// Min value for speed random variable
	float m_minSpeed;
	// Max value for speed random variable
	float m_maxSpeed;
	// How much parent's particle velocity is taken into new particle velocity
	float m_parentVelocityFactor;

	// Particle element data for position
	Tr2ParticleElementData m_positionElement;
	// Particle element data for velocity
	Tr2ParticleElementData m_velocityElement;
	// Was the generator successfully bound
	bool m_valid;
};

TYPEDEF_BLUECLASS( Tr2SphereShapeAttributeGenerator );

#endif // Tr2SphereShapeAttributeGenerator_H