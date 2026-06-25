// Copyright © 2016 CCP ehf.

#pragma once
#ifndef EveLocatorSets_H
#define EveLocatorSets_H

// decalre structured list here
struct Locator
{
	Vector3 position;
	Quaternion direction;
	Vector3 scale;
	int boneIndex;
};
BLUE_DECLARE_STRUCTURE_LIST( Locator );

// --------------------------------------------------------------------------------
// Description:
//   A set of locatorlists, identified by names
// --------------------------------------------------------------------------------
BLUE_CLASS( EveLocatorSets ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveLocatorSets( IRoot* lockobj = NULL );
	~EveLocatorSets();

	// access
	template <typename T>
	void Set( const char* name, const T* locators, size_t count );

	void Translate( const Vector3& offset );
	void Append( const Locator* locators, size_t count );

	bool HasName( const char* name ) const;
	bool HasName( const BlueSharedString& name ) const;
	const LocatorStructureList* GetLocators() const;
	const char* GetName() const;
	void SetName( BlueSharedString name );

private:
	// name to identify set
	BlueSharedString m_name;

	// the locators
	PLocatorStructureList m_locators;
};
TYPEDEF_BLUECLASS( EveLocatorSets );

template <typename T>
void EveLocatorSets::Set( const char* name, const T* locators, size_t count )
{
	m_name = BlueSharedString( name );
	m_locators.Clear();
	m_locators.Resize( count );
	for( int i = 0; i < count; ++i )
	{
		m_locators[i] = (Locator)locators[i];
	}
}


#endif