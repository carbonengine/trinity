// Copyright © 2010 CCP ehf.

#pragma once
#ifndef Tr2VariableStore_H
#define Tr2VariableStore_H

#include "Tr2DeviceResource.h"
#include "TriVariable.h"

BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );

// -------------------------------------------------------------
// Description:
//   Tr2VariableStore is a collection of named variables used by
//   shader system for binding. All instances of Tr2VariableStore
//   form an acyclic graph with GlobalStore() being the root.
// SeeAlso:
//   TriVariable
// -------------------------------------------------------------
BLUE_CLASS( Tr2VariableStore ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2VariableStore( IRoot* lockobj = 0 );
	~Tr2VariableStore();

	Tr2VariableStore* GetParentVariableStore() const;
	void SetParentVariableStore( Tr2VariableStore * variableStore );

	TriVariable* RegisterVariable( const char* name, float value );
	TriVariable* RegisterVariable( const char* name, int value );
	TriVariable* RegisterVariable( const char* name, const Vector2& value );
	TriVariable* RegisterVariable( const char* name, const Vector3& value );
	TriVariable* RegisterVariable( const char* name, const Vector4& value );
	TriVariable* RegisterVariable( const char* name, const Matrix& value );
	TriVariable* RegisterVariable( const char* name, const Color& value );
	TriVariable* RegisterVariable( const char* name );

	TriVariable* RegisterVariable( const char* name, ITr2TextureProvider* value );
	TriVariable* RegisterVariable( const char* name, ITr2GpuBuffer* value );
	TriVariable* RegisterVariable( const char* name, const Tr2TextureAL& value );

	void UnregisterVariable( const char* name );
	bool UnregisterLocalVariable( const char* name );
	TriVariable* FindVariable( const char* name ) const;
	TriVariable* FindLocalVariable( const char* name ) const;
	TriVariable* GetVariable( const char* name );
	TriVariable* GetLocalVariable( const char* name );

protected:
	Tr2VariableStore( IRoot * lockobj, int );

private:
	std::vector<std::string> GetLocalNames() const;

	TriVariable* RegisterVariableType( const char* name, TriVariableContentType type );

	// -------------------------------------------------------------
	// Description:
	//   Helper function to register a new variable.
	// Arguments:
	//   U - Template parameter: type of variable to register
	//   name - Name of the new variable
	//   value - Value for the new variable
	// -------------------------------------------------------------
	template <typename U>
	TriVariable* RegisterVariableInternal( const char* name, const U& value )
	{
		TriVariableContentType type = GetVariableType( value );
		TriVariable* var = RegisterVariableType( name, type );
		if( var )
		{
			var->SetValue( value );
		}
		return var;
	}

	// Pointer to the parent variable store
	// used to resolve variables
	Tr2VariableStorePtr m_parentVariableStore;
	// Map of variables in this store

	TrackableStdUnorderedMap<std::string, TriVariablePtr> m_variableMap;
};

TYPEDEF_BLUECLASS( Tr2VariableStore );
BLUE_DECLARE_VECTOR( Tr2VariableStore );

Tr2VariableStore& GlobalStore();

#endif // Tr2VariableStore_H