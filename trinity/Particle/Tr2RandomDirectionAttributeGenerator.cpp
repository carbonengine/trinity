// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2RandomDirectionAttributeGenerator.h"
#include "Tr2ParticleSystem.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2RandomDirectionAttributeGenerator default constructor
// --------------------------------------------------------------------------------------
Tr2RandomDirectionAttributeGenerator::Tr2RandomDirectionAttributeGenerator() :
	m_name( Tr2ParticleElementDeclarationName::CUSTOM ),
	m_valid( false ),
	m_element()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RandomDirectionAttributeGenerator destructor
// --------------------------------------------------------------------------------------
Tr2RandomDirectionAttributeGenerator::~Tr2RandomDirectionAttributeGenerator()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2AttributeGenerator interface. Generates unit-length random vector
//   for new particle component (element).
// Arguments:
//   position - Position of the "parent" particle (unused).
//   velocity - Velocity of the "parent" particle (unused).
//   paticle - (out) New particle data: Tr2ParticleElementData::COUNT of float arrays.
//		The generator fills element identified by generator's m_name with random values.
// --------------------------------------------------------------------------------------
void Tr2RandomDirectionAttributeGenerator::Generate( const Vector3* position,
													 const Vector3* velocity,
													 float** particle )
{
	if( !m_valid )
	{
		return;
	}
	Vector4 randomDirection( 0.0f, 0.0f, 0.0f, 0.0f );
	float length = 0;
	{
		for( unsigned i = 0; i < m_element.m_dimension; ++i )
		{
			randomDirection[i] = -1.f + 2.f * Tr2ParticleSystem::RandFloat();
			length += randomDirection[i] * randomDirection[i];
		}
	}
	if( length == 0.0f )
	{
		randomDirection.x = 1.0f;
	}
	else
	{
		randomDirection /= sqrt( length );
	}
	float* data = particle[m_element.m_bufferType] + m_element.m_offset;
	for( unsigned i = 0; i < m_element.m_dimension; ++i )
	{
		data[i] = randomDirection[i];
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   "Binds" generator to a particle system. Searches for particle element with name
//   equal to m_name.
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
bool Tr2RandomDirectionAttributeGenerator::Bind(
	const Tr2ParticleElementDataMap& declaration,
	std::set<Tr2ParticleElementDeclarationName>& boundElements )
{
	m_valid = false;
	auto i = declaration.find( m_name );
	if( i != declaration.end() )
	{
		if( boundElements.find( m_name ) != boundElements.end() )
		{
			CCP_LOGERR( "Multiple bindings to particle element %s", m_name.GetName().c_str() );
			return false;
		}
		boundElements.insert( m_name );
		m_element = i->second;
		m_valid = true;
		return true;
	}
	CCP_LOGERR( "Could not find particle element %s in particle system", m_name.GetName().c_str() );
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns human-readable name for generator's declaration element. Used for Python
//   exposure.
// Return Value:
//   Human-readable name of particle declaration element.
// --------------------------------------------------------------------------------------
std::string Tr2RandomDirectionAttributeGenerator::GetName() const
{
	return m_name.GetName();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns bounded particle element dimension or 0 if the generator is not bounded.
//   Used for Python exposure.
// Return Value:
//   Bounded particle element dimension or 0 if the generator is not bounded.
// --------------------------------------------------------------------------------------
unsigned Tr2RandomDirectionAttributeGenerator::GetDimension() const
{
	return m_valid ? m_element.m_dimension : 0;
}
