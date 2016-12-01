////////////////////////////////////////////////////////////
//
//    Created:   December 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "Tr2VariableStore.h"

Tr2VariableStore::Tr2VariableStore( IRoot* lockobj )
{
	SetParentVariableStore( &GlobalStore() );
}

// -------------------------------------------------------------
// Description:
//   A special protected constructor for global variable store
//   that avoids reeterancy of GlobalStore function.
// See also:
//   Tr2GlobalVariableStore
// -------------------------------------------------------------
Tr2VariableStore::Tr2VariableStore( IRoot* lockobj, int )
{
}

Tr2VariableStore::~Tr2VariableStore()
{
	ReleaseResources( TRISTORAGE_ALL );
    VariableMap::iterator end = m_variableMap.end();
	for( VariableMap::iterator it = m_variableMap.begin(); it != end; ++it )
	{
		TriVariable* var = *it;
		var->~TriVariable();
		CCP_FREE( var );
	}
}

// -------------------------------------------------------------
// Description:
//   Returns the parent variable store used during variable 
//   search. GlobalStore() is the default parent of new
//   variable store objects.
// Return Value:
//   Parent variable store.
// -------------------------------------------------------------
Tr2VariableStore* Tr2VariableStore::GetParentVariableStore() const
{
	return m_parentVariableStore;
}

// -------------------------------------------------------------
// Description:
//   Assigns a parent to the variable store. 
// Arguments:
//   variableStore - New parent of the variable store
// -------------------------------------------------------------
void Tr2VariableStore::SetParentVariableStore(Tr2VariableStore* variableStore)
{
	if( this == &GlobalStore() )
	{
		return;
	}

#if TRINITYDEV
	// Check if we don't assign a child as a parent
	Tr2VariableStore *store = variableStore;
	while( store )
	{
		CCP_ASSERT( store != this );
		store = store->GetParentVariableStore();
	}
#endif
	m_parentVariableStore = variableStore;
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable, but does not set a type for it.
//	 
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterPlaceholderTextureVariable( const char* name )
{
	return RegisterVariableType( name, TRIVARIABLE_UNKNOWN_TEXTURE );
}

TriVariable* Tr2VariableStore::RegisterVariable( const char* name, Tr2TextureAL* value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, float value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, int value )
{
	return RegisterVariableInternal( name, value );
}


// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, const Vector2& value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, const Vector3& value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, const Vector4& value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, const Matrix& value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, const Color& value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name, const IRoot* value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Registers a placeholder for a variable. 
// Arguments:
//   name - Name of the new variable
//   value - Value for the new variable
// Return Value:
//   New variable (or old with the same name).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariable( const char* name )
{
	return RegisterVariableType( name, TRIVARIABLE_INVALID );
}

TriVariable* Tr2VariableStore::RegisterVariable( const char* name, ITr2TextureProvider* value )
{
	return RegisterVariableInternal( name, value );
}

TriVariable* Tr2VariableStore::RegisterVariable( const char* name, ITr2GpuBuffer* value )
{
	return RegisterVariableInternal( name, value );
}

// -------------------------------------------------------------
// Description:
//   Unregisters a variable with the given name in this store
//   or in the first parent that has it.
// Arguments:
//   name - Name of the variable to unregister.
// -------------------------------------------------------------
void Tr2VariableStore::UnregisterVariable( const char* name )
{
	Tr2VariableStore* store = this;
	while( store )
	{
		if( store->UnregisterLocalVariable( name ) )
		{
			return;
		}
		store = store->GetParentVariableStore();
	}
}

// -------------------------------------------------------------
// Description:
//   Unregisters a variable with the given name in this store.
// Arguments:
//   name - Name of the variable to unregister.
// Return Value:
//   true If the variable was found.
//   false Otherwise.
// -------------------------------------------------------------
bool Tr2VariableStore::UnregisterLocalVariable( const char* name )
{
	if( !name )
	{
		return false;
	}
	CTriVariable key;
	key.m_name = name;
    auto it = m_variableMap.find( &key );
    if( it != m_variableMap.end() )
    {
		TriVariable* var = *it;

        m_variableMap.erase( it );

		// Wipe value memory area to zero
		var->Invalidate();

		// We leak these variables on purpose so we can reliably catch when they are
		// being accessed post unregistration!  This also prevents us from crashing if
		// somebody has a dangling reference to it.
		return true;
    }
	return false;
}

// -------------------------------------------------------------
// Description:
//   Searches for a variable the given name in this store and its
//   parents.
// Arguments:
//   name - Name of the variable to unregister.
// Return Value:
//   Variable with the given name or NULL if it was not found.
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::FindVariable( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const Tr2VariableStore* store = this;
	while( store )
	{
		if( TriVariable* variable = store->FindLocalVariable( name ) )
		{
			return variable;
		}
		store = store->GetParentVariableStore();
	}
	return NULL;
}

// -------------------------------------------------------------
// Description:
//   Searches for a variable the given name in this store.
// Arguments:
//   name - Name of the variable to unregister.
// Return Value:
//   Variable with the given name or NULL if it was not found.
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::FindLocalVariable( const char* name ) const
{
	if( !name )
	{
		return nullptr;
	}
	CTriVariable key;
	key.m_name = name;
    auto it = m_variableMap.find( &key );

	return it != m_variableMap.end() ? *it : nullptr;
}

// -------------------------------------------------------------
// Description:
//   Searches for a variable the given name in this store and its
//   parents. If the variable is not found the function registers
//   it in this store (with type TRIVARIABLE_INVALID).
// Arguments:
//   name - Name of the variable to unregister.
// Return Value:
//   Variable with the given name.
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::GetVariable( const char* name )
{
	Tr2VariableStore* store = this;
	while( store )
	{
		if( TriVariable* variable = store->FindLocalVariable( name ) )
		{
			return variable;
		}
		store = store->GetParentVariableStore();
	}
	
	void* buffer = CCP_MALLOC( "TriVariable", 
		sizeof( TriVariable ) 
		+ TriVariable::GetTypeSize( TRIVARIABLE_FLOAT4X4 ) // Largest variable
		- sizeof(uint32_t) );
	// Use placement new to initialize instance into memory area that is potentially
	// bigger than class size:
	TriVariable* var = new( buffer ) CTriVariable; 
	var->m_type = TRIVARIABLE_INVALID;
	var->m_name = name;
	m_variableMap.insert( var );
	return var;
}

// -------------------------------------------------------------
// Description:
//   Searches for a variable the given name in this store. If the 
//   variable is not found the function registers
//   it in this store (with type TRIVARIABLE_INVALID).
// Arguments:
//   name - Name of the variable to unregister.
// Return Value:
//   Variable with the given name.
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::GetLocalVariable( const char* name )
{
	if( !name )
	{
		return nullptr;
	}
	CTriVariable key;
	key.m_name = name;
    auto it = m_variableMap.find( &key );
    if( it != m_variableMap.end() )
    {
		return *it;
	}
	
	void* buffer = CCP_MALLOC( "TriVariable", 
		sizeof( TriVariable ) 
		+ TriVariable::GetTypeSize( TRIVARIABLE_FLOAT4X4 ) // Largest variable
		- sizeof(uint32_t) );
	// Use placement new to initialize instance into memory area that is potentially
	// bigger than class size:
	TriVariable* var = new( buffer ) CTriVariable; 
	var->m_type = TRIVARIABLE_INVALID;
	var->m_name = name;
	m_variableMap.insert( var );
	return var;
}

// -------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Releases all texture variables
//   in the store.
// Arguments:
//   s - Storage class of resources to release.
// -------------------------------------------------------------
void Tr2VariableStore::ReleaseResources( TriStorage s )
{
	// Clean out the texture handles (invalidate them)
    VariableMap::iterator end = m_variableMap.end();
	for( VariableMap::iterator it = m_variableMap.begin(); it != end; ++it )
	{
		TriVariable* var = *it;
		if( var->GetType() == TRIVARIABLE_TEXTURE_AL )
		{
			var->Clear();
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Variable store can't restore
//   its textures, so this function does nothing.
// Return Value:
//   true Always.
// -------------------------------------------------------------
bool Tr2VariableStore::OnPrepareResources()
{
	return true;
}

// -------------------------------------------------------------
// Description:
//   Registers a new variable. If the variable with that name
//   is already registered in this store then if its type is the
//   same the variable is reused otherwise the error is logged
//   and the function returns NULL.
// Arguments:
//   name - Name of the new variable
//   type - Type of the new variable
// Return Value:
//   New variable (or old with the same name or NULL if function
//   fails).
// -------------------------------------------------------------
TriVariable* Tr2VariableStore::RegisterVariableType( const char* name, TriVariableContentType type )
{
	TriVariable* var = FindLocalVariable( name );

	if( var )
	{
		// Variable already exists, ensure type matches
		TriVariableContentType existingType = var->GetType();
		if( existingType == TRIVARIABLE_INVALID )
		{
			// Variable was just reserved, switch it to this type, 
			// it has enough allocated space
			var->m_type = type;
		}
		else if( (existingType == TRIVARIABLE_UNKNOWN_TEXTURE) && ( type == TRIVARIABLE_TEXTURE_RES || type == TRIVARIABLE_TEXTURE_AL ) )
		{
			// Allow resolution to one of the defined the texture variable types
			var->m_type = type;
		}
		else if( (type == TRIVARIABLE_UNKNOWN_TEXTURE) && ( existingType == TRIVARIABLE_TEXTURE_RES || existingType == TRIVARIABLE_TEXTURE_AL ) )
		{
			// Trying to re-register a generic unknown texture type, which has already been narrowed down to RES or AL. Leave it alone.
		}
		else if( type == TRIVARIABLE_TEXTURE_RES && existingType == TRIVARIABLE_TEXTURE_AL )
		{
			var->m_type = type;
		}
		else if( ( type == TRIVARIABLE_TEXTURE_AL && existingType == TRIVARIABLE_TEXTURE_RES ) )
		{
			var->m_type = type;
			var->m_texture = nullptr;
		}
		else if( type == TRIVARIABLE_INVALID )
		{
			return var;
		}
		else if( type != existingType )
		{
			// Variable exists under a different type
			CCP_LOGERR( "Attempting to register variable '%s' as '%s', already registered as '%s'", name, 
				TriVariable::GetTypeName( type ), TriVariable::GetTypeName( existingType ) );
			CCP_ASSERT( false );
			var = NULL;
		}
	}
	else
	{
		// Create new variable
		void* buffer = CCP_MALLOC( "TriVariable", 
			sizeof( TriVariable ) + TriVariable::GetTypeSize( type ) - sizeof(uint32_t) );
		// Use placement new to initialize instance into memory area that is potentially
		// bigger than class size:
		var = new( buffer ) CTriVariable; 
		var->m_type = type;
		var->m_name = name;
		m_variableMap.insert( var );
	}

	return var;
}

std::vector<std::string> Tr2VariableStore::GetLocalNames() const
{
	std::vector<std::string> result;
	for( auto it = m_variableMap.cbegin(); it != m_variableMap.cend(); ++it )
	{
		result.push_back( ( *it )->GetName() );
	}
	return result;
}

class Tr2GlobalVariableStore: public Tr2VariableStore
{
public:
	Tr2GlobalVariableStore( IRoot* lockobj = nullptr )
		:Tr2VariableStore( lockobj, 0 )
	{

	}
};

Tr2VariableStore& GlobalStore()
{
	static RootNoLock<Tr2GlobalVariableStore> variableStore;
	return variableStore;
}
