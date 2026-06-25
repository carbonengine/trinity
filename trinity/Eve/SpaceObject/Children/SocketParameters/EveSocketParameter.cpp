// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveSocketParameter.h"

#include "Tr2ExternalParameter.h"
#include "TriValueBinding.h"

#if BLUE_WITH_PYTHON
#include <BluePyCpp.h>
#endif


EveSocketParameterBindingBase::EveSocketParameterBindingBase( IRoot* lockobj ) :
	PARENTLOCK( m_bindings ),
	m_name()
{
}

EveSocketParameterBindingBase::~EveSocketParameterBindingBase()
{
}

void EveSocketParameterBindingBase::ClearBindings()
{
	m_bindings.Clear();
}

bool EveSocketParameterBindingBase::BindToExternalParameter( Tr2ExternalParameter& externalParameter )
{
	if( !externalParameter.IsValid() || externalParameter.GetName() != m_name )
	{
		return false;
	}

	TriValueBindingPtr binding = externalParameter.CreateBinding();
	// the value attribute of any derived class should be exposed in blue as "value" to make this work...
	binding->SetSource( "value", this );
	binding->Initialize();
	if( !binding->IsValid() )
	{
		return false;
	}

	if( !ExtractDefault( externalParameter ) )
	{
		return false;
	}
	m_bindings.Append( dynamic_cast<ITr2ValueBinding*>( binding.Detach() ) );

	return true;
}

bool EveSocketParameterBindingBase::Used() const
{
	return !m_bindings.empty();
}

void EveSocketParameterBindingBase::Propagate()
{
	for( auto it = begin( m_bindings ); it != end( m_bindings ); ++it )
	{
		( *it )->CopyValue();
	}
}

namespace
{
bool GetExternalParameterValue( bool& value, const Tr2ExternalParameter& externalParameter )
{
	if( auto entry = externalParameter.GetDestinationEntry() )
	{
		auto src = externalParameter.GetDestination();
		if( !src )
		{
			return false;
		}
		switch( entry->mType )
		{
		case Be::LONG:
			value = *reinterpret_cast<const int32_t*>( src ) != 0;
			return true;
		case Be::FLOAT:
			value = *reinterpret_cast<const float*>( src ) != 0.0f;
			return true;
		case Be::DOUBLE:
			value = *reinterpret_cast<const double*>( src ) != 0.0;
			return true;
		case Be::BOOL:
			value = *reinterpret_cast<const bool*>( src );
			return true;
		case Be::INT64:
			value = *reinterpret_cast<const int64_t*>( src ) != 0;
			return true;
		case Be::BYTE:
			value = *reinterpret_cast<const uint8_t*>( src ) != 0;
			return true;
		case Be::SHORT:
			value = *reinterpret_cast<const int16_t*>( src ) != 0;
			return true;
		default:
			return false;
		}
	}
	return false;
}

bool GetExternalParameterValue( int& value, const Tr2ExternalParameter& externalParameter )
{
	if( auto entry = externalParameter.GetDestinationEntry() )
	{
		auto src = externalParameter.GetDestination();
		if( !src )
		{
			return false;
		}
		switch( entry->mType )
		{
		case Be::LONG:
			value = *reinterpret_cast<const int32_t*>( src );
			return true;
		case Be::FLOAT:
			value = int( *reinterpret_cast<const float*>( src ) );
			return true;
		case Be::DOUBLE:
			value = int( *reinterpret_cast<const double*>( src ) );
			return true;
		case Be::BOOL:
			value = *reinterpret_cast<const bool*>( src ) ? 1 : 0;
			return true;
		case Be::INT64:
			value = int( *reinterpret_cast<const int64_t*>( src ) );
			return true;
		case Be::BYTE:
			value = int( *reinterpret_cast<const uint8_t*>( src ) );
			return true;
		case Be::SHORT:
			value = *reinterpret_cast<const int16_t*>( src );
			return true;
		default:
			return false;
		}
	}
	return false;
}

bool GetExternalParameterValue( float& value, const Tr2ExternalParameter& externalParameter )
{
	if( auto entry = externalParameter.GetDestinationEntry() )
	{
		auto src = externalParameter.GetDestination();
		if( !src )
		{
			return false;
		}
		switch( entry->mType )
		{
		case Be::LONG:
			value = float( *reinterpret_cast<const int32_t*>( src ) );
			return true;
		case Be::FLOAT:
			value = *reinterpret_cast<const float*>( src );
			return true;
		case Be::DOUBLE:
			value = float( *reinterpret_cast<const double*>( src ) );
			return true;
		case Be::BOOL:
			value = *reinterpret_cast<const bool*>( src ) ? 1.0f : 0.0f;
			return true;
		case Be::INT64:
			value = float( *reinterpret_cast<const int64_t*>( src ) );
			return true;
		case Be::BYTE:
			value = float( *reinterpret_cast<const uint8_t*>( src ) );
			return true;
		case Be::SHORT:
			value = float( *reinterpret_cast<const int16_t*>( src ) );
			return true;
		default:
			return false;
		}
	}
	return false;
}

bool GetExternalParameterValue( Vector2& value, const Tr2ExternalParameter& externalParameter )
{
	if( auto entry = externalParameter.GetDestinationEntry() )
	{
		auto src = externalParameter.GetDestination();
		if( !src )
		{
			return false;
		}
		switch( entry->mType )
		{
		case Be::FLOATARRAY:
			if( entry->GetFloatArraySize() == 2 )
			{
				auto v = reinterpret_cast<const float*>( src );
				value = Vector2( v[0], v[1] );
				return true;
			}
			else
			{
				return false;
			}
		case Be::DOUBLEARRAY:
			if( entry->GetDoubleArraySize() == 2 )
			{
				auto v = reinterpret_cast<const double*>( src );
				value = Vector2( float( v[0] ), float( v[1] ) );
				return true;
			}
			else
			{
				return false;
			}
		case Be::INTARRAY:
			if( entry->GetIntArraySize() == 2 )
			{
				auto v = reinterpret_cast<const int*>( src );
				value = Vector2( float( v[0] ), float( v[1] ) );
				return true;
			}
			else
			{
				return false;
			}
		default:
			return false;
		}
	}
	return false;
}

bool GetExternalParameterValue( Vector3& value, const Tr2ExternalParameter& externalParameter )
{
	if( auto entry = externalParameter.GetDestinationEntry() )
	{
		auto src = externalParameter.GetDestination();
		if( !src )
		{
			return false;
		}
		switch( entry->mType )
		{
		case Be::FLOATARRAY:
			if( entry->GetFloatArraySize() == 3 )
			{
				auto v = reinterpret_cast<const float*>( src );
				value = Vector3( v[0], v[1], v[2] );
				return true;
			}
			else
			{
				return false;
			}
		case Be::DOUBLEARRAY:
			if( entry->GetDoubleArraySize() == 3 )
			{
				auto v = reinterpret_cast<const double*>( src );
				value = Vector3( float( v[0] ), float( v[1] ), float( v[2] ) );
				return true;
			}
			else
			{
				return false;
			}
		case Be::INTARRAY:
			if( entry->GetIntArraySize() == 3 )
			{
				auto v = reinterpret_cast<const int*>( src );
				value = Vector3( float( v[0] ), float( v[1] ), float( v[2] ) );
				return true;
			}
			else
			{
				return false;
			}
		default:
			return false;
		}
	}
	return false;
}

bool GetExternalParameterValue( Vector4& value, const Tr2ExternalParameter& externalParameter )
{
	if( auto entry = externalParameter.GetDestinationEntry() )
	{
		auto src = externalParameter.GetDestination();
		if( !src )
		{
			return false;
		}
		switch( entry->mType )
		{
		case Be::FLOATARRAY:
			if( entry->GetFloatArraySize() == 4 )
			{
				auto v = reinterpret_cast<const float*>( src );
				value = Vector4( v[0], v[1], v[2], v[3] );
				return true;
			}
			else
			{
				return false;
			}
		case Be::DOUBLEARRAY:
			if( entry->GetDoubleArraySize() == 4 )
			{
				auto v = reinterpret_cast<const double*>( src );
				value = Vector4( float( v[0] ), float( v[1] ), float( v[2] ), float( v[3] ) );
				return true;
			}
			else
			{
				return false;
			}
		case Be::INTARRAY:
			if( entry->GetIntArraySize() == 4 )
			{
				auto v = reinterpret_cast<const int*>( src );
				value = Vector4( float( v[0] ), float( v[1] ), float( v[2] ), float( v[3] ) );
				return true;
			}
			else
			{
				return false;
			}
		default:
			return false;
		}
	}
	return false;
}


bool GetExternalParameterValue( Color& value, const Tr2ExternalParameter& externalParameter )
{
	Vector4 v;
	if( GetExternalParameterValue( v, externalParameter ) )
	{
		value = Color( v );
		return true;
	}
	return false;
}


}

#define SOCKET_PARAMETER_DEFINE( _className, _valueType, _defaultValue )             \
	_className::_className( IRoot* lockobj ) :                                       \
		EveSocketParameterBindingBase( lockobj ),                                    \
		m_defaults(),                                                                \
		m_value( _defaultValue )                                                     \
	{                                                                                \
	}                                                                                \
	_className::~_className()                                                        \
	{                                                                                \
	}                                                                                \
	void _className::ClearBindings()                                                 \
	{                                                                                \
		m_defaults.clear();                                                          \
		EveSocketParameterBindingBase::ClearBindings();                              \
	}                                                                                \
	void _className::Reset()                                                         \
	{                                                                                \
		for( size_t i = 0; i < m_bindings.size(); ++i )                              \
		{                                                                            \
			m_value = m_defaults[i];                                                 \
			m_bindings[i]->CopyValue();                                              \
		}                                                                            \
		ClearBindings();                                                             \
	}                                                                                \
	bool _className::ExtractDefault( const Tr2ExternalParameter& externalParameter ) \
	{                                                                                \
		_valueType value;                                                            \
		if( GetExternalParameterValue( value, externalParameter ) )                  \
		{                                                                            \
			m_defaults.push_back( value );                                           \
		}                                                                            \
		else                                                                         \
		{                                                                            \
			m_defaults.push_back( _defaultValue );                                   \
		}                                                                            \
		return true;                                                                 \
	}                                                                                \
	void _className::SetValueToDefault()                                             \
	{                                                                                \
		if( !m_defaults.empty() )                                                    \
		{                                                                            \
			m_value = m_defaults[0];                                                 \
		}                                                                            \
		else                                                                         \
		{                                                                            \
			m_value = _defaultValue;                                                 \
		}                                                                            \
	}

SOCKET_PARAMETER_DEFINE( EveSocketParameterBool, bool, false );
SOCKET_PARAMETER_DEFINE( EveSocketParameterInt, int, 0 );
SOCKET_PARAMETER_DEFINE( EveSocketParameterFloat, float, 0.0f );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector2, Vector2, Vector2( 0, 0 ) );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector3, Vector3, Vector3( 0, 0, 0 ) );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector4, Vector4, Vector4( 0, 0, 0, 0 ) );
SOCKET_PARAMETER_DEFINE( EveSocketParameterColor, Color, Color( 0, 0, 0, 0 ) );

EveSocketParameterString::EveSocketParameterString( IRoot* lockobj ) :
	PARENTLOCK( m_externalParameters ),
	m_name( "" ),
	m_value( "" ),
	m_valueExposure(),
	m_defaults()
{
}

EveSocketParameterString::~EveSocketParameterString()
{
}

bool EveSocketParameterString::Initialize()
{
	if( !m_valueExposure )
	{
		m_valueExposure.CreateInstance();
		m_valueExposure->SetName( "valueExposure" );
		m_valueExposure->SetDestinationObject( this );
		m_valueExposure->SetDestinationAttribute( "value" );
		m_valueExposure->Initialize();
	}
	return true;
}

void EveSocketParameterString::ClearBindings()
{
	m_externalParameters.Clear();
}

bool EveSocketParameterString::BindToExternalParameter( Tr2ExternalParameter& externalParameter )
{
	Initialize();
	if( !externalParameter.IsValid() || externalParameter.GetName() != m_name )
	{
		return false;
	}

	if( !ExtractDefault( externalParameter ) )
	{
		return false;
	}
	m_externalParameters.Append( externalParameter.GetRawRoot() );

	return true;
}

bool EveSocketParameterString::ExtractDefault( const Tr2ExternalParameter& externalParameter )
{
	std::string value;
	BlueScriptValue blueValue;
	externalParameter.GetValue( blueValue );
	if( BlueExtractArgument( blueValue, value, 0 ) )
	{
		m_defaults.push_back( value );
	}
	else
	{
		m_defaults.push_back( "" );
	}
	return true;
}

void EveSocketParameterString::SetValueToDefault()
{
	if( !m_defaults.empty() )
	{
		m_value = m_defaults[0];
	}
}

bool EveSocketParameterString::Used() const
{
	return !m_externalParameters.empty();
}

void EveSocketParameterString::Propagate()
{
	if( m_valueExposure && m_valueExposure->IsValid() )
	{
		BlueScriptValue v;
		m_valueExposure->GetValue( v );
		for( auto it = begin( m_externalParameters ); it != end( m_externalParameters ); ++it )
		{
			( *it )->SetValue( v );
		}
	}
}