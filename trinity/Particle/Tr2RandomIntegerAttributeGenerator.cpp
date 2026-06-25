// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2RandomIntegerAttributeGenerator.h"
#include "Tr2ParticleSystem.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2RandomIntegerAttributeGenerator default constructor
// --------------------------------------------------------------------------------------
Tr2RandomIntegerAttributeGenerator::Tr2RandomIntegerAttributeGenerator() :
	m_name( Tr2ParticleElementDeclarationName::CUSTOM ),
	m_minRange( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_maxRange( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_valid( false ),
	m_element()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RandomIntegerAttributeGenerator destructor
// --------------------------------------------------------------------------------------
Tr2RandomIntegerAttributeGenerator::~Tr2RandomIntegerAttributeGenerator()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2AttributeGenerator interface. Generates random integer values for new
//   particle component (element).
// Arguments:
//   position - Position of the "parent" particle (unused).
//   velocity - Velocity of the "parent" particle (unused).
//   paticle - (out) New particle data: Tr2ParticleElementData::COUNT of float arrays.
//		The generator fills element identified by generator's m_name with random values.
// --------------------------------------------------------------------------------------
void Tr2RandomIntegerAttributeGenerator::Generate( const Vector3* position,
												   const Vector3* velocity,
												   float** particle )
{
	if( !m_valid )
	{
		return;
	}
	float* data = particle[m_element.m_bufferType] + m_element.m_offset;
	for( unsigned i = 0; i < m_element.m_dimension; ++i )
	{
		data[i] = floor( m_minRange[i] + ( m_maxRange[i] - m_minRange[i] ) * Tr2ParticleSystem::RandFloat() + 0.5f );
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
bool Tr2RandomIntegerAttributeGenerator::Bind(
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
std::string Tr2RandomIntegerAttributeGenerator::GetName() const
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
unsigned Tr2RandomIntegerAttributeGenerator::GetDimension() const
{
	return m_valid ? m_element.m_dimension : 0;
}
