////////////////////////////////////////////////////////////
//
//    Creator:   Reevan McKay
//    Created:   December 2010
//    Copyright: CCP 2010-2011
//

#include "StdAfx.h"

#include "Tr2MaterialParameterStore.h"

// --------------------------------------------------------------------------------------
Tr2MaterialParameterStore::Tr2MaterialParameterStore( IRoot* lockobj ):
	PARENTLOCK( m_parameters )
{
}

// --------------------------------------------------------------------------------------
Tr2MaterialParameterStore::~Tr2MaterialParameterStore()
{
}


// --------------------------------------------------------------------------------------
bool Tr2MaterialParameterStore::Initialize()
{
	LoadParentResource();

	return true;
}

// --------------------------------------------------------------------------------------
bool Tr2MaterialParameterStore::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_parentPath ) )
	{
		LoadParentResource();
	}

	return true;
}

void Tr2MaterialParameterStore::LoadParentResource()
{
	if( m_parentStore )
		m_parentStore.Unlock();

	m_parentStore = NULL;
	
	if (m_parentPath.size() > 0)
	{
		m_parentStore = BeResMan->LoadObject<Tr2MaterialParameterStore>( m_parentPath.c_str() );
	}

}

// --------------------------------------------------------------------------------------
//	Find a named material parameter in us or our parent store
//	Return Null if not found
// --------------------------------------------------------------------------------------
ITriEffectParameter* Tr2MaterialParameterStore::FindParameter( const char* name )
{
	Tr2MaterialParameterStore *currentStore = this;

	while( currentStore != NULL )
	{
		PITriEffectParameterDict::const_iterator it = currentStore->m_parameters.find( name );

		if( it != currentStore->m_parameters.end() )
			return it->second;

		currentStore = currentStore->m_parentStore;
	}

	return NULL;
}