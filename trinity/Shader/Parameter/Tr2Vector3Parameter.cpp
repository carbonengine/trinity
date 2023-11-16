#include "StdAfx.h"

#include "Tr2Vector3Parameter.h"
#include "TriValueBinding.h"
#include "Shader/Tr2Shader.h"

// --------------------------------------------------------------------------------------
// Description:
//   Constructor.  Initializes member variables to defaults.
// --------------------------------------------------------------------------------------
Tr2Vector3Parameter::Tr2Vector3Parameter(IRoot* lockobj) :
	m_isUsedByEffect( false ),
	m_isSrgb( false ),
	m_value( 1.0f, 1.0f, 1.0f ),
	m_linearValue( 1.0f, 1.0f, 1.0f ),
	m_reroutedValue( NULL ),
	m_bindings( "Tr2Vector3Parameter/m_bindings" )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the parameter name as a c-string.
// Return Value:
//   Parameter name as a c-string
// --------------------------------------------------------------------------------------
const char* Tr2Vector3Parameter::GetParameterName() const
{
	return m_name.c_str();
}

unsigned Tr2Vector3Parameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), CcpHashFNV1( &m_value, sizeof( m_value ), startingHash ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies the vector3 value to the destination buffer.
// Arguments:
//   destHandle - Pointer to the destination buffer
//   size - Number of bytes to copy (could differ from the size of a vector3)
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t size,
												Tr2RenderContext &renderContext ) const
{
	// We need this to work even when the effect we're copying this to isn't the one 
	// that we're bound to
	if( m_reroutedValue )
	{
		memcpy( 
			destHandle, 
			m_reroutedValue, 
			size < sizeof(m_value) ? size : sizeof(m_value) 
		);
	}
	else if( m_isSrgb )
	{
		memcpy( destHandle, &m_linearValue, size < sizeof(m_value) ? size : sizeof( m_value ) );
	}
	else
	{
		memcpy( destHandle, &m_value, size < sizeof(m_value) ? size : sizeof( m_value ) );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the value of the parameter.  If the parameter is rerouted, it sets the
//   rerouted value as well.
// Arguments:
//   value - The new value of the parameter
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::SetValue( const Vector3& value )
{
	m_value = value;

	if( m_isSrgb )
	{
		m_linearValue = TriGammaToLinear( m_value );
	}

	if( m_reroutedValue )
	{
		m_reroutedValue[0] = value.x;
		m_reroutedValue[1] = value.y;
		m_reroutedValue[2] = value.z;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Used to get the value of the parameter.  If the parameter is rerouted, gets the value
//   of the rerouted parameter.
// Return Value:
//   The Vector3 value of this parameter
// --------------------------------------------------------------------------------------
const Vector3& Tr2Vector3Parameter::GetValue( void )
{
	if( m_reroutedValue )
	{
		m_value.x = m_reroutedValue[0];
		m_value.y = m_reroutedValue[1];
		m_value.z = m_reroutedValue[2];
	}

	return m_value;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query whether the vector3 parameter is rerouted
// Return Value:
//   true, if the parameter is rerouted
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2Vector3Parameter::IsRerouted() const
{
	return !m_isSrgb && m_reroutedValue != NULL;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the destination buffer and buffer size
// Arguments:
//   dest - The destination buffer
//   size - The size of the destination buffer in bytes
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::SetDestination( void* dest, size_t size )
{
	if( size >= sizeof( float ) * 3 && !m_isSrgb )
	{
		m_reroutedValue = (float*)dest;
		m_reroutedValue[0] = m_value.x;
		m_reroutedValue[1] = m_value.y;
		m_reroutedValue[2] = m_value.z;

		for( BindingVector_t::iterator it = m_bindings.begin(); it != m_bindings.end(); ++it )
		{
			(*it)->RerouteDestination( m_reroutedValue );
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
			(*it)->RerouteDestination( &m_value );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the destination buffer and its size
// Arguments:
//   dest - Handle to the destination buffer (output)
//   size - Size of the destination buffer (output)
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::GetDestination( void*& dest, size_t& size )
{
	if( m_reroutedValue )
	{
		dest = m_reroutedValue;
	}
	else
	{
		dest = (void*)&m_value;
	}
	size = sizeof( float ) * 3;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a value binding.
// Arguments:
//   vb - Value binding
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::RegisterBinding( TriValueBinding* vb )
{
	CCP_ASSERT( vb );

	// Note that this is a weak reference - adding a reference here would 
	// create a circular reference.
	m_bindings.push_back( vb );
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a value binding
// Arguments:
//   vb - The value binding to remove
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::UnregisterBinding( TriValueBinding* vb )
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

// --------------------------------------------------------------------------------------
// Description:
//   Rebuilds the effect handles.  This basically determines whether the parameter is
//   used by the current effect and technique.
// Arguments:
//   effect - The current effect
//   technique - The current technique
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_isSrgb = false;

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

	const Tr2EffectConstant* constant = effectRes->GetConstant( m_name.c_str() );

	if ( !constant )
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

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting X-component of Vector3.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector3 X-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The X-component of the Vector3
// --------------------------------------------------------------------------------------
float Tr2Vector3Parameter::GetX( void )
{
	if( m_reroutedValue )
	{
		m_value.x = m_reroutedValue[0];
	}

	return m_value.x;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting Y-component of Vector3.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector3 X-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The Y-component of the Vector3
// --------------------------------------------------------------------------------------
float Tr2Vector3Parameter::GetY( void )
{
	if( m_reroutedValue )
	{
		m_value.y = m_reroutedValue[1];
	}

	return m_value.y;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting Z-component of Vector3.  This is used to setup a Blue
//   property.  If the value is rerouted, it refreshes the Vector3 Z-component from the
//   rerouted buffer before returning the value.
// Return Value:
//   The Z-component of the Vector3
// --------------------------------------------------------------------------------------
float Tr2Vector3Parameter::GetZ( void )
{
	if( m_reroutedValue )
	{
		m_value.z = m_reroutedValue[2];
	}

	return m_value.z;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for setting X-component of Vector3.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   x - The new X-component value
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::SetX( float x )
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
//   Utility function for setting Y-component of Vector3.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   y - The new Y-component value
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::SetY( float y )
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
//   Utility function for setting Z-component of Vector3.  This is used to setup a Blue
//   property.  If the value is rerouted, the argument is assigned in the rerouted
//   buffer as well.
// Arguments:
//   z - The new Z-component value
// --------------------------------------------------------------------------------------
void Tr2Vector3Parameter::SetZ( float z )
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

bool Tr2Vector3Parameter::Initialize()
{
	// This gets called when using CopyTo - make sure rerouted value gets updated.
	if( m_reroutedValue )
	{
		m_reroutedValue[0] = m_value.x;
		m_reroutedValue[1] = m_value.y;
		m_reroutedValue[2] = m_value.z;
	}
	if( m_isSrgb )
	{
		m_linearValue = TriGammaToLinear( m_value );
	}

	return true;
}
