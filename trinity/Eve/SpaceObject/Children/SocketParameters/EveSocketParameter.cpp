////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
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
	if ( !externalParameter.IsValid() || externalParameter.GetName() != m_name )
	{
		return false;
	}

	TriValueBindingPtr binding = externalParameter.CreateBinding();
	// the value attribute of any derived class should be exposed in blue as "value" to make this work...
	binding->SetSource( "value", this );
	binding->Initialize();
	if ( !binding->IsValid() )
	{
		return false;
	}
	
	if ( !ExtractDefault( externalParameter ) )
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
	for ( auto it = begin( m_bindings ); it != end( m_bindings ); ++it )
	{
		( *it )->CopyValue();
	}
}

#if BLUE_WITH_PYTHON
#define SOCKET_PARAMETER_GIL_LOCK Ccp::PyGilEnsure gilWrapper
#define SOCKET_PARAMETER_CLEAR_ERROR PyErr_Clear()
#else
#define SOCKET_PARAMETER_GIL_LOCK
#define SOCKET_PARAMETER_CLEAR_ERROR
#endif

#define SOCKET_PARAMETER_DEFINE( _className, _valueType, _defaultValue )\
	_className::_className( IRoot* lockobj ) :\
		EveSocketParameterBindingBase( lockobj ),\
		m_defaults( ),\
		m_value( _defaultValue )\
	{\
	}\
	_className::~_className()\
	{\
	}\
	void _className::ClearBindings()\
	{\
		m_defaults.clear();\
		EveSocketParameterBindingBase::ClearBindings();\
	}\
	void _className::Reset()\
	{\
		for ( size_t i = 0; i < m_bindings.size(); ++i )\
		{\
			m_value = m_defaults[i];\
			m_bindings[i]->CopyValue();\
		}\
		ClearBindings();\
	}\
	bool _className::ExtractDefault( const Tr2ExternalParameter& externalParameter )\
	{\
		SOCKET_PARAMETER_GIL_LOCK;\
		_valueType value;\
		BlueScriptValue blueValue;\
		externalParameter.GetValue( blueValue );\
		if ( BlueExtractArgument( blueValue, value, 0 ) )\
		{\
			m_defaults.push_back( value );\
		}\
		else\
		{\
			SOCKET_PARAMETER_CLEAR_ERROR;\
			m_defaults.push_back( _defaultValue );\
		}\
		return true;\
	}\
	void _className::SetValueToDefault()\
	{\
		if (!m_defaults.empty())\
		{\
			m_value = m_defaults[0];\
		}\
		else\
		{\
			m_value = _defaultValue;\
		}\
	}\

SOCKET_PARAMETER_DEFINE( EveSocketParameterBool, bool, false );
SOCKET_PARAMETER_DEFINE( EveSocketParameterInt, int, 0 );
SOCKET_PARAMETER_DEFINE( EveSocketParameterFloat, float, 0.0f );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector2, Vector2, Vector2() );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector3, Vector3, Vector3() );
SOCKET_PARAMETER_DEFINE( EveSocketParameterVector4, Vector4, Vector4() );
SOCKET_PARAMETER_DEFINE( EveSocketParameterColor, Color, Color() );

EveSocketParameterString::EveSocketParameterString( IRoot* lockobj ) :
	PARENTLOCK( m_externalParameters ),
	m_name(""),
	m_value(""),
	m_valueExposure(),
	m_defaults()
{
}

EveSocketParameterString::~EveSocketParameterString()
{
}

bool EveSocketParameterString::Initialize()
{
	if ( !m_valueExposure )
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
	if ( !externalParameter.IsValid() || externalParameter.GetName() != m_name )
	{
		return false;
	}

	if ( !ExtractDefault( externalParameter ) )
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
	if ( BlueExtractArgument( blueValue, value, 0 ) )
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
	if (!m_defaults.empty())
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
	if ( m_valueExposure && m_valueExposure->IsValid() )
	{
		BlueScriptValue v;
		m_valueExposure->GetValue( v );
		for ( auto it = begin( m_externalParameters ); it != end( m_externalParameters ); ++it )
		{
			( *it )->SetValue( v );
		}
	}
}