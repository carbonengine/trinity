////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2ConsecutiveIntegerAttributeGenerator.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2RandomIntegerAttributeGenerator default constructor
// --------------------------------------------------------------------------------------
Tr2ConsecutiveIntegerAttributeGenerator::Tr2ConsecutiveIntegerAttributeGenerator()
	:m_name( Tr2ParticleElementDeclarationName::CUSTOM ),
	m_minRange( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_maxRange( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_valid( false ),
	m_element()
{
	for( int i = 0; i < 4; ++i )
	{
		m_currentValues[i] = 0;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RandomIntegerAttributeGenerator destructor
// --------------------------------------------------------------------------------------
Tr2ConsecutiveIntegerAttributeGenerator::~Tr2ConsecutiveIntegerAttributeGenerator()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2AttributeGenerator interface. Generates consecutive integer values 
//   for new particle component (element).
// Arguments:
//   position - Position of the "parent" particle (unused).
//   velocity - Velocity of the "parent" particle (unused).
//   paticle - (out) New particle data: Tr2ParticleElementData::COUNT of float arrays. 
//		The generator fills element identified by generator's m_name with random values.
// --------------------------------------------------------------------------------------
void Tr2ConsecutiveIntegerAttributeGenerator::Generate( const Vector3* position, 
												   const Vector3* velocity, 
												   float** particle )
{
	if( !m_valid )
	{
		return;
	}
	float* data = particle[m_element.m_bufferType] + m_element.m_offset;
	float* minRange = &m_minRange.x;
	float* maxRange = &m_maxRange.x;
	for( unsigned i = 0; i < m_element.m_dimension; ++i )
	{
		m_currentValues[i]++;
		auto r = uint32_t( maxRange[i] - minRange[i] );
		if( r )
		{
			data[i] = float( minRange[i] + m_currentValues[i] % r );
		}
		else
		{
			data[i] = minRange[i];
		}
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
bool Tr2ConsecutiveIntegerAttributeGenerator::Bind( 
	const Tr2ParticleElementDataMap& declaration, 
	std::set<Tr2ParticleElementDeclarationName> &boundElements )
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
std::string Tr2ConsecutiveIntegerAttributeGenerator::GetName() const
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
unsigned Tr2ConsecutiveIntegerAttributeGenerator::GetDimension() const
{
	return m_valid ? m_element.m_dimension : 0;
}
