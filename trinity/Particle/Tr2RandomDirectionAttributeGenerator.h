////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2RandomDirectionAttributeGenerator_H
#define Tr2RandomDirectionAttributeGenerator_H

#include "ITr2AttributeGenerator.h"

// --------------------------------------------------------------------------------------
// Description:
//   Particle element generator that generates a random unit-length vector.
// See Also:
//   ITr2AttributeGenerator, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2RandomDirectionAttributeGenerator ): public ITr2AttributeGenerator
{
public:
	EXPOSE_TO_BLUE();

	Tr2RandomDirectionAttributeGenerator();
	~Tr2RandomDirectionAttributeGenerator();

	void Generate( const Vector3* position, const Vector3* velocity, float** particle );
	bool Bind( const Tr2ParticleElementDataMap& declaration, std::set<Tr2ParticleElementDeclarationName> &boundElements );
	std::string GetName() const;
	unsigned GetDimension() const;
private:
	// Particle element type/name
	Tr2ParticleElementDeclarationName m_name;

	// Element data the generator is bound to
	Tr2ParticleElementData m_element;
	// Was the generator successfully bound
	bool m_valid;
};

TYPEDEF_BLUECLASS( Tr2RandomDirectionAttributeGenerator );

#endif // Tr2RandomDirectionAttributeGenerator_H