#include "StdAfx.h"
#include "Tr2FloatParameter.h"
#include "TriValueBinding.h"
#include "Shader/Tr2Shader.h"

Tr2FloatParameter::Tr2FloatParameter(IRoot* lockobj):
	m_value( 1.0f ),
	m_name(),
	m_isUsedByEffect( false ),
	m_bindings( "Tr2FloatParameter::m_bindings" ),
	m_reroutedValue( NULL )
{
}

Tr2FloatParameter::~Tr2FloatParameter()
{
}

const char* Tr2FloatParameter::GetParameterName() const
{
	return m_name.c_str();
}

unsigned Tr2FloatParameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), CcpHashFNV1( &m_value, sizeof( m_value ), startingHash ) );
}

void Tr2FloatParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
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

float Tr2FloatParameter::GetValue()
{
	if( m_reroutedValue )
	{
		m_value = *m_reroutedValue;
	}

	return m_value;
}

void Tr2FloatParameter::SetValue( float val )
{
	m_value = val;
	if( m_reroutedValue )
	{
		*m_reroutedValue = m_value;
	}
}

bool Tr2FloatParameter::IsRerouted() const
{
	return m_reroutedValue != NULL;
}

void Tr2FloatParameter::SetDestination( void* dest, size_t size )
{
	if( size >= sizeof( float ) )
	{
		m_reroutedValue = (float*)dest;
		*m_reroutedValue = m_value;

		for( auto it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			( *it )->RerouteDestination( m_reroutedValue );
		}
	}
	else
	{
		m_reroutedValue = NULL;

		for( auto it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			( *it )->RerouteDestination( &m_value );
		}
	}
}

void Tr2FloatParameter::GetDestination( void*& dest, size_t& size )
{
	if( m_reroutedValue )
	{
		dest = m_reroutedValue;
	}
	else
	{
		dest = (void*)&m_value;
	}
	size = sizeof( float );
}

void Tr2FloatParameter::RegisterBinding( TriValueBinding* vb )
{
	CCP_ASSERT( std::find( m_bindings.begin(), m_bindings.end(), vb ) == m_bindings.end() );
	m_bindings.push_back( vb );
}

void Tr2FloatParameter::UnregisterBinding( TriValueBinding* vb )
{
	auto found = std::find( m_bindings.begin(), m_bindings.end(), vb );
	if( found != m_bindings.end() )
	{
		m_bindings.erase( found );
	}
}

void Tr2FloatParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	if( !effectRes && m_reroutedValue )
	{
		// Ensure that rerouted values are not left with a dangling reference.
		SetDestination( NULL, 0 );
	}
	m_isUsedByEffect = ( effectRes && !m_name.empty() && effectRes->GetConstant( m_name.c_str() ) );
}

bool Tr2FloatParameter::Initialize()
{
	// This gets called when using CopyTo - make sure rerouted value gets updated.
	if( m_reroutedValue )
	{
		*m_reroutedValue = m_value;
	}
	return true;
}
