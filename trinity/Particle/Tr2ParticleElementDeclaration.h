////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2ParticleElementDeclaration_H
#define Tr2ParticleElementDeclaration_H

BLUE_DECLARE( Tr2ParticleElementDeclaration );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleElementDeclarationName is a structure used to identify particle element.
//   It consists of a semantic type and a string name (used when type is CUSTOM).
// See Also:
//   Tr2ParticleElementDeclaration, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
struct Tr2ParticleElementDeclarationName
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Semantic type of a particle element.
	// ----------------------------------------------------------------------------------
	enum Type
	{
		// Element represents particle age (in X component) and lifetime (in Y component)
		LIFETIME,
		// Element represents particle position
		POSITION,
		// Element represents particle velocity
		VELOCITY,
		// Element represents particle mass
		MASS,
		// Custom particle element
		CUSTOM,
	};

	Tr2ParticleElementDeclarationName( Type type );
	Tr2ParticleElementDeclarationName( Type type, std::string name );

	bool operator==( const Tr2ParticleElementDeclarationName& other ) const;
	bool operator<( const Tr2ParticleElementDeclarationName& other ) const;
	std::string GetName() const;
	Tr2VertexDefinition::UsageCode GetD3DUsage() const;

	// Element semantic type
	Type m_type;
	// Custom element name
	std::string m_name;
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleElementData stores all relevant information for a single particle element.
// See Also:
//   Tr2ParticleSystem
// --------------------------------------------------------------------------------------
struct Tr2ParticleElementData
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Type of the buffer where particle element is stored.
	// ----------------------------------------------------------------------------------
	enum BufferType
	{
		// Buffer that is copied to GPU vertex buffer
		GPU,
		// CPU-only buffer
		CPU,

		// Number of buffers
		COUNT,
	};

	// Element dimension
	unsigned m_dimension;
	// Element usage index
	unsigned m_usageIndex;
	// Buffer type which the element belongs to
	BufferType m_bufferType;
	// Offset in the buffer
	unsigned m_offset;

	static Tr2ParticleElementData Invalid()
	{
		Tr2ParticleElementData none;
		none.m_dimension = 0;
		none.m_usageIndex = 0;
		none.m_bufferType = Tr2ParticleElementData::COUNT;
		none.m_offset = 0;
		return none;
	}
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleStreamIterator is a helper class to manage a stream of typed data. Used
//   internally be particle system components. 
// See Also:
//   Tr2ParticleElementDeclaration, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
template<typename T>
class Tr2ParticleStreamIterator
{
public:
	// ----------------------------------------------------------------------------------
	// Description:
	//   Constructs an iterator.
	// Arguments:
	//   particle - Array of pointers to particle data; number of elements in array is 
	//		Tr2ParticleElementData::COUNT.
	//   strides - Array of strides (number of floats) per particle in particle arrays.
	//   element - Particle element of interest.
	// ----------------------------------------------------------------------------------
	Tr2ParticleStreamIterator( float** particle, unsigned* strides, const Tr2ParticleElementData& element )
		:m_data( particle[element.m_bufferType] + element.m_offset ),
		m_stride( strides[element.m_bufferType] )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Advances iterator to the next particle.
	// Return value:
	//   This object.
	// ----------------------------------------------------------------------------------
	Tr2ParticleStreamIterator& operator++()
	{
		m_data += m_stride;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Advances iterator by a given offset.
	// Arguments:
	//   offset - Offset
	// Return value:
	//   This object.
	// ----------------------------------------------------------------------------------
	Tr2ParticleStreamIterator& operator+=( int offset )
	{
		m_data += m_stride * offset;
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Access data of the particle element.
	// Return value:
	//   Pointer to particle element data.
	// ----------------------------------------------------------------------------------
	operator T*()
	{
		return reinterpret_cast<T*>( m_data );
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Access data of the particle element.
	// Return value:
	//   Pointer to particle element data.
	// ----------------------------------------------------------------------------------
	T* Get()
	{
		return reinterpret_cast<T*>( m_data );
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Checks if particle element data is 16 byte aligned.
	// Return value:
	//   true If particle element data is 16 byte aligned.
	//   false Otherwise.
	// ----------------------------------------------------------------------------------
	bool Is16ByteAligned() const
	{
		return ( uintptr_t( m_data ) & 15 ) == 0;
	}
private:
	// Pointer to the current particle element data
	float* m_data;
	// Particle data stride (in floats)
	unsigned m_stride;
};

// --------------------------------------------------------------------------------------
// Description:
//   Map from particle element name to its description data.
// See Also:
//   Tr2ParticleSystem
// --------------------------------------------------------------------------------------
typedef std::map<Tr2ParticleElementDeclarationName, Tr2ParticleElementData> 
		Tr2ParticleElementDataMap;

// --------------------------------------------------------------------------------------
// Description:
//   Python-exposed particle element declaration type. Specifies all data for a single
//   particle element. Tr2ParticleSystem uses a list of Tr2ParticleElementDeclaration to
//   compile a Tr2ParticleElementDataMap of particle element data.
// See Also:
//   Tr2ParticleSystem
// --------------------------------------------------------------------------------------
class Tr2ParticleElementDeclaration: public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleElementDeclaration( IRoot* lockobj = 0 );

	unsigned GetSize() const;
	std::string GetName() const;

	// Element type/name
	Tr2ParticleElementDeclarationName m_name;
	// Element dimension (from 1 to 4)
	uint32_t m_dimension;
	// Custom type element usage index (for shader binding)
	uint32_t m_usageIndex;
	// Is the element exposed to shaders
	bool m_usedByGPU;
};

TYPEDEF_BLUECLASS( Tr2ParticleElementDeclaration );
BLUE_DECLARE_VECTOR( Tr2ParticleElementDeclaration );

#endif // Tr2ParticleElementDeclaration_H