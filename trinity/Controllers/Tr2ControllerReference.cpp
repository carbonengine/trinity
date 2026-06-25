// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ControllerReference.h"


Tr2ControllerReference::Tr2ControllerReference( IRoot* ) :
	m_owner( nullptr )
{
}

bool Tr2ControllerReference::Initialize()
{
	if( !m_path.empty() )
	{
		m_controller = BeResMan->LoadObject<ITr2Controller>( m_path.c_str() );
	}
	return true;
}

bool Tr2ControllerReference::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_path ) )
	{
		m_controller = nullptr;
		if( !m_path.empty() )
		{
			m_controller = BeResMan->LoadObject<ITr2Controller>( m_path.c_str() );
			if( m_controller && m_owner )
			{
				m_controller->Link( *m_owner );
			}
		}
	}
	return true;
}

void Tr2ControllerReference::Link( IRoot& owner )
{
	m_owner = &owner;
	if( m_controller )
	{
		m_controller->Link( owner );
	}
}

void Tr2ControllerReference::Unlink( UnlinkReason reason )
{
	m_owner = nullptr;
	if( m_controller )
	{
		m_controller->Unlink( reason );
	}
}

bool Tr2ControllerReference::IsLinked() const
{
	return m_owner != nullptr;
}


void Tr2ControllerReference::Start()
{
	if( m_controller )
	{
		m_controller->Start();
	}
}

void Tr2ControllerReference::Stop()
{
	if( m_controller )
	{
		m_controller->Stop();
	}
}

void Tr2ControllerReference::Update( float normalizedUpdateFrequency )
{
	if( m_controller )
	{
		m_controller->Update( normalizedUpdateFrequency );
	}
}

void Tr2ControllerReference::SetVariable( const char* name, float value )
{
	if( m_controller )
	{
		m_controller->SetVariable( name, value );
	}
}

void Tr2ControllerReference::HandleEvent( const char* eventName )
{
	if( m_controller )
	{
		m_controller->HandleEvent( eventName );
	}
}

IRoot* Tr2ControllerReference::GetOwner() const
{
	return m_owner;
}