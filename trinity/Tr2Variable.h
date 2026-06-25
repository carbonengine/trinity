// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2Variable_h_
#define Tr2Variable_h_

#include "TriVariable.h"
#include "Tr2VariableStore.h"

// Simple helper class to make the syntax for registering variables and assigning values
// a little easier, and more importantly to make sure that every access properly checks
// for a null variable, which isn't the case with TriVariable* fooHandle at various places.
class Tr2Variable
{
private:
	//Tr2VariableStore	*m_store;
	TriVariable* m_variable;

	Tr2Variable( const Tr2Variable& );
	Tr2Variable& operator=( const Tr2Variable& );

public:
	Tr2Variable() :
		/*m_store( nullptr )
	, */
		m_variable( nullptr )
	{
	}

	explicit Tr2Variable( TriVariable* variable ) :
		m_variable( variable )
	{
	}

	template <typename TYPE>
	Tr2Variable( const char* name, TYPE value )
	{
		Register( name, value, GlobalStore() );
	}

	template <typename TYPE>
	Tr2Variable( const char* name, TYPE value, Tr2VariableStore& store )
	{
		Register( name, value, store );
	}

	template <typename TYPE>
	void Register( const char* name, TYPE value )
	{
		Register( name, value, GlobalStore() );
	}

	template <typename TYPE>
	void Register( const char* name, TYPE value, Tr2VariableStore& store )
	{
		m_variable = store.RegisterVariable( name, value );
	}

	template <typename TYPE>
	void GetValue( TYPE& value )
	{
		if( m_variable )
		{
			m_variable->GetValue( value );
		}
	}

	template <typename TYPE>
	void operator=( const TYPE& value )
	{
		if( m_variable )
		{
			m_variable->SetValue( value );
		}
	}

	TriVariable* GetVariable() const
	{
		return m_variable;
	}

	TriVariableContentType GetType() const
	{
		return m_variable ? m_variable->GetType() : TRIVARIABLE_INVALID;
	}

	const char* GetTypeName() const
	{
		return m_variable ? m_variable->GetTypeName() : "null var";
	}

	// Invalidate the variable
	void Invalidate()
	{
		if( m_variable )
		{
			m_variable->Invalidate();
		}
	}

	// Clear the value, but leave the type alone, so a new SetValue will still work.
	void Clear()
	{
		if( m_variable )
		{
			m_variable->Clear();
		}
	}
};

#endif // Tr2Variable_h_
