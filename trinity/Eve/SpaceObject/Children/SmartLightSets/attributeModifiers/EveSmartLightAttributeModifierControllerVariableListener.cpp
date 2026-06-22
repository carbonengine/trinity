// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierControllerVariableListener.h"

EveSmartLightAttributeModifierControllerVariableListener::EveSmartLightAttributeModifierControllerVariableListener( IRoot* lockobj ) :
	m_variableName( "" ),
	m_value( 0.f ),
	m_invertReceivedValue( false ),
	m_defaultValue( 0.f )
{
	m_name = "ControllerVariableListener";
}

bool EveSmartLightAttributeModifierControllerVariableListener::Initialize()
{
	m_value = m_defaultValue;
	m_startsActive = m_defaultValue > 0.5f;
	m_active = m_defaultValue > 0.5f;
	return EveSmartLightBaseAttributeModifier::Initialize();
}

bool EveSmartLightAttributeModifierControllerVariableListener::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_value ) || IsMatch( value, m_invertReceivedValue ) )
	{
		if( m_invertReceivedValue )
		{
			this->SetActive( m_value < 1.f );
		}
		else
		{
			this->SetActive( m_value > 0.f );
		}
	}

	EveSmartLightBaseAttributeModifier::OnModified( value );
	return true;
}

void EveSmartLightAttributeModifierControllerVariableListener::SetControllerVariable( const char* name, float value )
{
	if( m_variableName == name )
	{
		m_value = value;
		if( m_invertReceivedValue )
		{
			this->SetActive( m_value < 1.f );
		}
		else
		{
			this->SetActive( m_value > 0.f );
		}
	}

	for( auto modifier : m_attributeModifiers )
	{
		modifier->SetControllerVariable( name, value );
	}
}
