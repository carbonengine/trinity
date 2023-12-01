////////////////////////////////////////////////////////////
//
//    Created:   Februaru 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "EveLocatorSets.h"

// locator item definition
static BlueStructureDefinition LocatorStructureDef[] =
{
	{ "position", Be::FLOAT32_3, 0 },
	{ "direction", Be::FLOAT32_4, 12 },
	{ "boneIndex", Be::INT32_1, 28 },
	{ 0 }
};

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveLocatorSets::EveLocatorSets( IRoot* lockobj ) :
	PARENTLOCK( m_locators )
{
	m_locators.SetStructureDefinition( LocatorStructureDef );
}

// --------------------------------------------------------------------------------
// Description:
//   Byebye
// --------------------------------------------------------------------------------
EveLocatorSets::~EveLocatorSets()
{
}

// --------------------------------------------------------------------------------
void EveLocatorSets::Translate( const Vector3& offset )
{
	if( LengthSq(offset) == 0.0f )
	{
		return;
	}
	for( auto it = m_locators.begin(); it != m_locators.end(); ++it )
	{
		it->position += offset;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Merge this locatorset with another set
// --------------------------------------------------------------------------------
void EveLocatorSets::Append( const Locator* locators, size_t count )
{
	size_t originalSize = m_locators.size();
	m_locators.Resize( originalSize + count );
	memcpy( &m_locators[ originalSize ], locators, count * sizeof( Locator ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Compare names
// --------------------------------------------------------------------------------
bool EveLocatorSets::HasName( const char* name ) const
{
	return ( m_name == BlueSharedString( name ) );
}

// --------------------------------------------------------------------------------
bool EveLocatorSets::HasName( const BlueSharedString& name ) const
{
	return m_name == name;
}

// --------------------------------------------------------------------------------
// Description:
//   Give out pointer to list
// --------------------------------------------------------------------------------
const LocatorStructureList* EveLocatorSets::GetLocators() const
{
	return &m_locators;
}

const char* EveLocatorSets::GetName() const
{
	return m_name.c_str();
}

void EveLocatorSets::SetName( BlueSharedString name )
{
	m_name = name;
}
