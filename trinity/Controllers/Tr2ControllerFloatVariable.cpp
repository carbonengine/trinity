// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ControllerFloatVariable.h"


Tr2ControllerFloatVariable::Tr2ControllerFloatVariable( IRoot* ) :
	m_destination( nullptr ),
	m_dirtyMaskDestination( nullptr ),
	m_dirtyMask( 0 ),
	m_type( FLOAT ),
	m_value( 0 ),
	m_defaultValue( 0 )
{
}

bool Tr2ControllerFloatVariable::Initialize()
{
	m_value = m_defaultValue;
	return true;
}

bool Tr2ControllerFloatVariable::OnModified( Be::Var* value )
{
	if( m_destination )
	{
		*m_destination = m_value;
	}
	if( m_dirtyMaskDestination )
	{
		*m_dirtyMaskDestination |= m_dirtyMask;
	}
	return true;
}

const std::string& Tr2ControllerFloatVariable::GetName() const
{
	return m_name;
}

float Tr2ControllerFloatVariable::GetValue() const
{
	return m_value;
}

void Tr2ControllerFloatVariable::SetValue( float value )
{
	m_value = value;
	if( m_destination )
	{
		*m_destination = value;
	}
	if( m_dirtyMaskDestination )
	{
		*m_dirtyMaskDestination |= m_dirtyMask;
	}
}

void Tr2ControllerFloatVariable::SetDestinationBuffer( float* buffer )
{
	m_destination = buffer;
	if( m_destination )
	{
		*m_destination = m_value;
	}
}

void Tr2ControllerFloatVariable::SetDirtyMask( uint64_t* maskDestination, uint64_t mask )
{
	m_dirtyMaskDestination = maskDestination;
	m_dirtyMask = mask;
}