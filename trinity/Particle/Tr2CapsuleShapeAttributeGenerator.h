// Copyright © 2012 CCP ehf.

#pragma once
#ifndef Tr2CapsuleShapeAttributeGenerator_H
#define Tr2CapsuleShapeAttributeGenerator_H

#include "ITr2AttributeGenerator.h"

// --------------------------------------------------------------------------------------
// Description:
//   Particle element generator that generates particle positions and/or velocities as if
//   particles are spawned in a moving sphere sector (with velocities pointing from the
//   sphere center).
// See Also:
//   ITr2AttributeGenerator, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2CapsuleShapeAttributeGenerator ) :
	public ITr2AttributeGenerator
{
public:
	EXPOSE_TO_BLUE();

	Tr2CapsuleShapeAttributeGenerator();
	~Tr2CapsuleShapeAttributeGenerator();

	void Generate( const Vector3* position, const Vector3* velocity, float** particle );
	bool Bind( const Tr2ParticleElementDataMap& declaration,
			   std::set<Tr2ParticleElementDeclarationName>& boundElements );
	std::string GetName() const;

	void SetPositions( const Vector3& startPosition,
					   const Quaternion& startRotation,
					   const Vector3& endPosition,
					   const Quaternion& endRotation );

private:
	// Generator positions
	Vector3 m_positionStart;
	Vector3 m_positionEnd;
	// Generator rotations
	Quaternion m_rotationStart;
	Quaternion m_rotationEnd;

	// Angles for emitting cones
	float m_minPhi, m_maxPhi;
	float m_minTheta, m_maxTheta;
	float m_minRadius, m_maxRadius;

	// Does the generator also generate velocities
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

TYPEDEF_BLUECLASS( Tr2CapsuleShapeAttributeGenerator );

#endif // Tr2CapsuleShapeAttributeGenerator_H