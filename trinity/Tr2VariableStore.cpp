////////////////////////////////////////////////////////////
//
//    Created:   December 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "Tr2VariableStore.h"

Tr2VariableStore::Tr2VariableStore( IRoot* lockobj )
	:m_variableMap( "Tr2VariableStore::m_variableMap" )
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
	:m_variableMap( "Tr2VariableStore::m_variableMap" )
{
}

Tr2VariableStore::~Tr2VariableStore()
{
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
	auto it = m_variableMap.find( name );
    if( it != m_variableMap.end() )
    {
		it->second->Invalidate();
        m_variableMap.erase( it );
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
    auto it = m_variableMap.find( name );

	return it != m_variableMap.end() ? it->second : nullptr;
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

	TriVariablePtr var;
	var.CreateInstance();
	var->m_type = TRIVARIABLE_INVALID;
	var->m_name = name;
	m_variableMap[name] = var;
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
    auto it = m_variableMap.find( name );
    if( it != m_variableMap.end() )
    {
		return it->second;
	}
	
	TriVariablePtr var;
	var.CreateInstance();
	var->m_type = TRIVARIABLE_INVALID;
	var->m_name = name;
	m_variableMap[name] = var;
	return var;
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
		TriVariablePtr variable;
		variable.CreateInstance();
		var = variable;
		var->m_type = type;
		var->m_name = name;
		m_variableMap[name] = var;
	}

	return var;
}

std::vector<std::string> Tr2VariableStore::GetLocalNames() const
{
	std::vector<std::string> result;
	for( auto it = m_variableMap.cbegin(); it != m_variableMap.cend(); ++it )
	{
		result.push_back( it->second->GetName() );
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
