#include "StdAfx.h"
#include "Tr2Matrix4Parameter.h"
#include "TriValueBinding.h"
#include "Shader/Tr2Shader.h"

Tr2Matrix4Parameter::Tr2Matrix4Parameter(IRoot* lockobj):
	m_isUsedByEffect( false ),
	m_value(	1.0f, 0, 0, 0, 
				0, 1.0f, 0, 0, 
				0, 0, 1.0f, 0, 
				0, 0, 0, 1.0f	),
	m_reroutedValue( NULL ),
	m_bindings( "Tr2Matrix4Parameter/m_bindings" )
{
}

const char* Tr2Matrix4Parameter::GetParameterName() const
{
	return m_name.c_str();
}

unsigned Tr2Matrix4Parameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), CcpHashFNV1( &m_value, sizeof( m_value ), startingHash ) );
}

void Tr2Matrix4Parameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t size,
												Tr2RenderContext &renderContext ) const
{
	// We need this to work even when the effect we're copying this to isn't the one that we're bound to
	if( m_reroutedValue )
	{
		memcpy( destHandle, m_reroutedValue, size < sizeof(m_value) ? size : sizeof(m_value) );
	}
	else
	{
		memcpy( destHandle, &m_value, size < sizeof(m_value) ? size : sizeof( m_value ) );
	}
}

Matrix	Tr2Matrix4Parameter::GetValue()
{
	if( m_reroutedValue )
	{
		memcpy( &m_value, m_reroutedValue, sizeof( m_value ) );		
	}

	return m_value;
}

void Tr2Matrix4Parameter::SetValue( const Matrix & value )
{
	m_value = value;

	if( m_reroutedValue )
	{
		memcpy( m_reroutedValue, &m_value, sizeof( m_value ) );
	}
}

bool Tr2Matrix4Parameter::IsRerouted() const
{
	return m_reroutedValue != NULL;
}

void Tr2Matrix4Parameter::SetDestination( void* dest, size_t size )
{
	if( size >= sizeof( m_value ) )
	{
		m_reroutedValue = (float*)dest;

		memcpy( m_reroutedValue, &m_value, sizeof( m_value ) );

		for( BindingVector_t::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			(*it)->RerouteDestination( m_reroutedValue );
		}
	}
	else
	{
		m_reroutedValue = NULL;

		for( BindingVector_t::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			(*it)->RerouteDestination( &m_value );
		}
	}
}

void Tr2Matrix4Parameter::GetDestination( void*& dest, size_t& size )
{
	if( m_reroutedValue )
	{
		dest = m_reroutedValue;
	}
	else
	{
		dest = (void*)&m_value;
	}
	size = sizeof( m_value );
}

void Tr2Matrix4Parameter::RegisterBinding( TriValueBinding* vb )
{
	CCP_ASSERT( vb );

	// Note that this is a weak reference - adding a reference here would 
	// create a circular reference.
	m_bindings.push_back( vb );
}

void Tr2Matrix4Parameter::UnregisterBinding( TriValueBinding* vb )
{
	CCP_ASSERT( vb );

	for( BindingVector_t::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
	{
		if( *it == vb )
		{
			m_bindings.erase( it );
			break;
		}
	}
}

//TODO this seems to be a bit of generic copy paste -- move to helper/base?
void Tr2Matrix4Parameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	if( !effectRes && m_reroutedValue )
	{
		// Ensure that rerouted values are not left with a dangling reference.
		SetDestination( NULL, 0 );
	}

	if ( m_name.empty() || !effectRes )
	{
		m_isUsedByEffect = false;
		return;
	}

	if ( !effectRes || !effectRes->GetConstant( m_name.c_str() ) )
	{
		m_isUsedByEffect = false;
	}
	else
	{
		m_isUsedByEffect = true;
	}
}
