// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleElementDeclaration.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleElementDeclarationName constructor.
// Arguments:
//   type - Element type to initialize new name with
// --------------------------------------------------------------------------------------
Tr2ParticleElementDeclarationName::Tr2ParticleElementDeclarationName( Type type ) :
	m_type( type )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleElementDeclarationName constructor.
// Arguments:
//   type - Element type to initialize new name with
//   name - Nameto initialize new name with (in case type is CUSTOM)
// --------------------------------------------------------------------------------------
Tr2ParticleElementDeclarationName::Tr2ParticleElementDeclarationName( Type type, std::string name ) :
	m_type( type ),
	m_name( name )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Equals operator for particle element name.
// Arguments:
//   other - Declaration name to compare with
// Return Value:
//   true If declaration names are equal
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleElementDeclarationName::operator==( const Tr2ParticleElementDeclarationName& other ) const
{
	return m_type == other.m_type && ( m_type != CUSTOM || m_name == other.m_name );
}

// --------------------------------------------------------------------------------------
// Description:
//   Less operator for particle element name (so that Tr2ParticleElementDeclarationName
//   can be used as keys in std::map). Specifies linear order on declaration names.
// Arguments:
//   other - Declaration name to compare with
// Return Value:
//   true If declaration name is "less" than other
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleElementDeclarationName::operator<( const Tr2ParticleElementDeclarationName& other ) const
{
	return m_type < other.m_type || ( m_type == CUSTOM && other.m_type == CUSTOM && m_name < other.m_name );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns human-readable name for declaration element. Used for Python exposure.
// Return Value:
//   Human-readable name of particle declaration element.
// --------------------------------------------------------------------------------------
std::string Tr2ParticleElementDeclarationName::GetName() const
{
	if( m_type >= CUSTOM )
	{
		return m_name;
	}
	static const char* typeNames[CUSTOM] = {
		"LIFETIME",
		"POSITION",
		"VELOCITY",
		"MASS",
	};
	return typeNames[m_type];
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns D3D declaration usage type for this particle element.
// Return Value:
//   D3D declaration usage type for this particle element.
// --------------------------------------------------------------------------------------
Tr2VertexDefinition::UsageCode Tr2ParticleElementDeclarationName::GetD3DUsage() const
{
	static const Tr2VertexDefinition::UsageCode usages[CUSTOM + 1] = {
		Tr2VertexDefinition::TANGENT,
		Tr2VertexDefinition::POSITION,
		Tr2VertexDefinition::NORMAL,
		Tr2VertexDefinition::BITANGENT,
		Tr2VertexDefinition::TEXCOORD,
	};
	return usages[m_type];
}


// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleElementDeclaration default constructor
// --------------------------------------------------------------------------------------
Tr2ParticleElementDeclaration::Tr2ParticleElementDeclaration( IRoot* lockobj ) :
	m_name( Tr2ParticleElementDeclarationName::CUSTOM ),
	m_dimension( 1 ),
	m_usageIndex( 0 ),
	m_usedByGPU( true )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns size (dimension) of an element. It is pre-defined for all element types
//   except for CUSTOM when m_dimension is used.
// Return Value:
//   Size (dimension) of an element.
// --------------------------------------------------------------------------------------
unsigned Tr2ParticleElementDeclaration::GetSize() const
{
	// We have pre-defined sizes for for all types except CUSTOM
	static const unsigned sizes[Tr2ParticleElementDeclarationName::CUSTOM] = {
		2,
		3,
		3,
		1,
	};
	if( m_name.m_type == Tr2ParticleElementDeclarationName::CUSTOM )
	{
		return m_dimension;
	}
	return sizes[m_name.m_type];
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns human-readable name for declaration element. Used for Python exposure.
// Return Value:
//   Human-readable name of particle declaration element.
// --------------------------------------------------------------------------------------
std::string Tr2ParticleElementDeclaration::GetName() const
{
	return m_name.GetName();
}