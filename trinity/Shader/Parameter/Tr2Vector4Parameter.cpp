// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Vector4Parameter.h"
#include "TriValueBinding.h"
#include "Shader/Tr2Shader.h"

Tr2Vector4Parameter::Tr2Vector4Parameter( IRoot* lockobj ) :
	m_isUsedByEffect( false ),
	m_isSrgb( false ),
	m_value( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_linearValue( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_reroutedValue( NULL ),
	m_bindings( "Tr2Vector4Parameter/m_bindings" )
{
}

const char* Tr2Vector4Parameter::GetParameterName() const
{
	return m_name.c_str();
}

unsigned Tr2Vector4Parameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), CcpHashFNV1( &m_value, sizeof( m_value ), startingHash ) );
}

void Tr2Vector4Parameter::CopyValueToEffect( Tr2RenderContextEnum::ShaderType inputType,
											 unsigned char* destHandle,
											 size_t size,
											 Tr2RenderContext& renderContext ) const
{
	// We need this to work even when the effect we're copying this to isn't the one that we're bound to
	if( m_reroutedValue )
	{
		memcpy( destHandle, m_reroutedValue, size < sizeof( m_value ) ? size : sizeof( m_value ) );
	}
	else if( m_isSrgb )
	{
		memcpy( destHandle, &m_linearValue, size < sizeof( m_value ) ? size : sizeof( m_value ) );
	}
	else
	{
		memcpy( destHandle, &m_value, size < sizeof( m_value ) ? size : sizeof( m_value ) );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Used to get the value of the parameter.  If the parameter is rerouted, gets the value
//   of the rerouted parameter.
// Return Value:
//   The Vector3 value of this parameter
// --------------------------------------------------------------------------------------
const Vector4& Tr2Vector4Parameter::GetValue( void )
{
	if( m_reroutedValue )
	{
		m_value.x = m_reroutedValue[0];
		m_value.y = m_reroutedValue[1];
		m_value.z = m_reroutedValue[2];
		m_value.w = m_reroutedValue[3];
	}

	return m_value;
}

// --------------------------------------------------------------------------------
// Description:
//   Used to set the value of the parameter
// Arguments:
//   value - The new value of the parameter
// --------------------------------------------------------------------------------
void Tr2Vector4Parameter::SetValue( const Vector4& value )
{
	m_value.x = value.x;
	m_value.y = value.y;
	m_value.z = value.z;
	m_value.w = value.w;

	if( m_isSrgb )
	{
		m_linearValue = TriGammaToLinear( m_value );
	}

	if( m_reroutedValue )
	{
		m_reroutedValue[0] = value.x;
		m_reroutedValue[1] = value.y;
		m_reroutedValue[2] = value.z;
		m_reroutedValue[3] = value.w;
	}
}


// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting X-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector4 X-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The X-component of the Vector4
// --------------------------------------------------------------------------------------
float Tr2Vector4Parameter::GetX( void )
{
	if( m_reroutedValue )
	{
		m_value.x = m_reroutedValue[0];
	}

	return m_value.x;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting Y-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector4 X-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The Y-component of the Vector4
// --------------------------------------------------------------------------------------
float Tr2Vector4Parameter::GetY( void )
{
	if( m_reroutedValue )
	{
		m_value.y = m_reroutedValue[1];
	}

	return m_value.y;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting Z-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector4 Z-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The Z-component of the Vector4
// --------------------------------------------------------------------------------------
float Tr2Vector4Parameter::GetZ( void )
{
	if( m_reroutedValue )
	{
		m_value.z = m_reroutedValue[2];
	}

	return m_value.z;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting W-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector4 W-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The W-component of the Vector4
// --------------------------------------------------------------------------------------
float Tr2Vector4Parameter::GetW( void )
{
	if( m_reroutedValue )
	{
		m_value.w = m_reroutedValue[3];
	}

	return m_value.w;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for setting X-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   x - The new X-component value
// --------------------------------------------------------------------------------------
void Tr2Vector4Parameter::SetX( float x )
{
	m_value.x = x;
	if( m_isSrgb )
	{
		m_linearValue.x = TriGammaToLinear( m_value.x );
	}

	if( m_reroutedValue )
	{
		m_reroutedValue[0] = x;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for setting Y-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   y - The new Y-component value
// --------------------------------------------------------------------------------------
void Tr2Vector4Parameter::SetY( float y )
{
	m_value.y = y;
	if( m_isSrgb )
	{
		m_linearValue.y = TriGammaToLinear( m_value.y );
	}

	if( m_reroutedValue )
	{
		m_reroutedValue[1] = y;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for setting Z-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   z - The new Z-component value
// --------------------------------------------------------------------------------------
void Tr2Vector4Parameter::SetZ( float z )
{
	m_value.z = z;
	if( m_isSrgb )
	{
		m_linearValue.z = TriGammaToLinear( m_value.z );
	}

	if( m_reroutedValue )
	{
		m_reroutedValue[2] = z;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for setting W-component of Vector4.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   w - The new W-component value
// --------------------------------------------------------------------------------------
void Tr2Vector4Parameter::SetW( float w )
{
	m_value.w = w;

	if( m_reroutedValue )
	{
		m_reroutedValue[3] = w;
	}
}




bool Tr2Vector4Parameter::IsRerouted() const
{
	return !m_isSrgb && m_reroutedValue != NULL;
}

void Tr2Vector4Parameter::SetDestination( void* dest, size_t size )
{
	if( size >= sizeof( float ) * 4 && !m_isSrgb )
	{
		m_reroutedValue = (float*)dest;
		m_reroutedValue[0] = m_value.x;
		m_reroutedValue[1] = m_value.y;
		m_reroutedValue[2] = m_value.z;
		m_reroutedValue[3] = m_value.w;

		for( BindingVector_t::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			( *it )->RerouteDestination( m_reroutedValue );
		}
	}
	else
	{
		m_reroutedValue = NULL;

		if( m_isSrgb )
		{
			m_linearValue = TriGammaToLinear( m_value );
		}

		for( BindingVector_t::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			( *it )->RerouteDestination( &m_value );
		}
	}
}

void Tr2Vector4Parameter::GetDestination( void*& dest, size_t& size )
{
	if( m_reroutedValue )
	{
		dest = m_reroutedValue;
	}
	else
	{
		dest = (void*)&m_value;
	}
	size = sizeof( float ) * 4;
}

void Tr2Vector4Parameter::RegisterBinding( TriValueBinding* vb )
{
	CCP_ASSERT( vb );

	// Note that this is a weak reference - adding a reference here would
	// create a circular reference.
	m_bindings.push_back( vb );
}

void Tr2Vector4Parameter::UnregisterBinding( TriValueBinding* vb )
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

void Tr2Vector4Parameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_isSrgb = false;

	if( !effectRes && m_reroutedValue )
	{
		// Ensure that rerouted values are not left with a dangling reference.
		SetDestination( NULL, 0 );
	}

	if( m_name.empty() || !effectRes )
	{
		m_isUsedByEffect = false;
		return;
	}

	const Tr2EffectConstant* constant = effectRes->GetConstant( m_name.c_str() );

	if( !constant )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;

	m_isSrgb = constant->isSRGB;

	if( m_isSrgb )
	{
		SetDestination( NULL, 0 );
	}
}

bool Tr2Vector4Parameter::Initialize()
{
	// This gets called when using CopyTo - make sure rerouted value gets updated.
	if( m_reroutedValue )
	{
		m_reroutedValue[0] = m_value.x;
		m_reroutedValue[1] = m_value.y;
		m_reroutedValue[2] = m_value.z;
		m_reroutedValue[3] = m_value.w;
	}
	if( m_isSrgb )
	{
		m_linearValue = TriGammaToLinear( m_value );
	}

	return true;
}
