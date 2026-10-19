// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "EveLocatorSets.h"
#include "Tr2GrannyAnimation.h"
#include "Utilities/MatrixUtils.h"

static_assert( sizeof( EveSpaceObjectChild::PartTag ) == sizeof( uint32_t ), "Size mismatch for PartTag: need to update LocatorStructureDef" );

// --------------------------------------------------------------------------------
// Description:
//   Get locator position and direction, transformed by animationUpdater.
//   We're assuming for now that the bone 0 isn't animated for performance reasons.
// --------------------------------------------------------------------------------
void EveGetLocatorPose( const Tr2GrannyAnimation* animationUpdater, const Locator& locator, Vector3& position, Vector3& direction )
{
	position = locator.position;
	direction = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), locator.direction );

	if( locator.boneIndex > 0 && animationUpdater && animationUpdater->IsInitialized() &&
		locator.boneIndex < animationUpdater->GetMeshBoneCount() )
	{
		const Float4x3* bones = animationUpdater->GetMeshBoneMatrixList();
		Matrix boneTF = IdentityMatrix();
		TriMatrixCopyFrom3x4( &boneTF, &bones[locator.boneIndex] );
		position = XMVector3TransformCoord( locator.position, boneTF );
		direction = XMVector3TransformNormal( direction, boneTF );
	}
}

// locator item definition
static BlueStructureDefinition LocatorStructureDef[] = {
	{ "position", Be::FLOAT32_3, 0 },
	{ "direction", Be::FLOAT32_4, 12 },
	{ "scale", Be::FLOAT32_3, 28 },
	{ "boneIndex", Be::INT32_1, 40 },
	{ "partTag", Be::UINT32_1, 44 },
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
	if( LengthSq( offset ) == 0.0f )
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
	memcpy( &m_locators[originalSize], locators, count * sizeof( Locator ) );
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

LocatorStructureList* EveLocatorSets::GetLocators()
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
