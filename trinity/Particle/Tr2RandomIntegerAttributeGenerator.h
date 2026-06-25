// Copyright © 2012 CCP ehf.

#pragma once
#ifndef Tr2RandomIntegerAttributeGenerator_H
#define Tr2RandomIntegerAttributeGenerator_H

#include "ITr2AttributeGenerator.h"

// --------------------------------------------------------------------------------------
// Description:
//   Particle element generator that creates random integer element values in a
//   specified range.
// See Also:
//   ITr2AttributeGenerator, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2RandomIntegerAttributeGenerator ) :
	public ITr2AttributeGenerator
{
public:
	EXPOSE_TO_BLUE();

	Tr2RandomIntegerAttributeGenerator();
	~Tr2RandomIntegerAttributeGenerator();

	void Generate( const Vector3* position, const Vector3* velocity, float** particle );
	bool Bind( const Tr2ParticleElementDataMap& declaration, std::set<Tr2ParticleElementDeclarationName>& boundElements );
	std::string GetName() const;
	unsigned GetDimension() const;

private:
	// Particle element type/name
	Tr2ParticleElementDeclarationName m_name;

	// Min range for random variable
	Vector4 m_minRange;
	// Max range for random variable
	Vector4 m_maxRange;

	// Element data the generator is bound to
	Tr2ParticleElementData m_element;
	// Was the generator successfully bound
	bool m_valid;
};

TYPEDEF_BLUECLASS( Tr2RandomIntegerAttributeGenerator );

#endif // Tr2RandomIntegerAttributeGenerator_H