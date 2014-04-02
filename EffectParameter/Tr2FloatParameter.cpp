#include "StdAfx.h"
#include "Tr2FloatParameter.h"
#include "TriValueBinding.h"
#include "ITr2ShaderState.h"

Tr2FloatParameter::Tr2FloatParameter(IRoot* lockobj):
	m_value( 1.0f ),
	m_name(),
	m_isUsedByEffect( false ),
	m_allowRerouting( true ),
	m_binding( NULL ),
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

// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the value of this float parameter is 0 and be ignored when
//   building the material situation.
// Return Value:
//   true, if m_value is within epsilon of 0
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2FloatParameter::IsZeroOrNull( void ) const
{
	static const float epsilon = 0.0000001f;

	return fabsf( m_value ) < epsilon;
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

size_t Tr2FloatParameter::GetValueSize() const
{
	return sizeof(m_value);
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
	return ( m_reroutedValue != NULL ) || !m_allowRerouting;
}

void Tr2FloatParameter::SetDestination( void* dest, size_t size )
{
	if( size >= sizeof( float ) )
	{
		m_reroutedValue = (float*)dest;
		*m_reroutedValue = m_value;

		if( m_binding )
		{
			m_binding->RerouteDestination( m_reroutedValue );
		}
	}
	else
	{
		m_reroutedValue = NULL;

		if( m_binding )
		{
			m_binding->RerouteDestination( &m_value );
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
	if( !m_allowRerouting )
	{
		return;
	}
	if( m_binding )
	{
		CCP_LOGWARN( "Tr2FloatParameter: detected a second value binding to the parameter; disabling routing" );
		m_allowRerouting = false;
		return;
	}

	// Note that this is a weak reference - adding a reference here would 
	// create a circular reference.
	m_binding = vb;
}

void Tr2FloatParameter::UnregisterBinding( TriValueBinding* vb )
{
	CCP_ASSERT( !m_binding || ( m_binding == vb ) || !m_allowRerouting );
	m_binding = NULL;
}

void Tr2FloatParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
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
