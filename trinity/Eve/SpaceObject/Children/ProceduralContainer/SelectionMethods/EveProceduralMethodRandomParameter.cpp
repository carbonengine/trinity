// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodRandomParameter.h"


EveProceduralMethodRandomParameter::EveProceduralMethodRandomParameter( IRoot* lockobj ) :
	m_weighting( 1 ),
	m_modified( false )
{
}

EveProceduralMethodRandomParameter::~EveProceduralMethodRandomParameter()
{
}

const char* EveProceduralMethodRandomParameter::GetName() const
{
	return m_name.c_str();
}

void EveProceduralMethodRandomParameter::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

bool EveProceduralMethodRandomParameter::Initialize()
{
	if( m_child == nullptr )
	{
		m_child.CreateInstance();
	}
	return true;
}

bool EveProceduralMethodRandomParameter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_weighting ) )
	{
		m_weighting = max( m_weighting, 1 );
		m_modified = true;
	}

	if( IsMatch( value, m_child ) )
	{
		if( m_child != nullptr )
		{
			m_child->SetAutoLoadBlocker( true );
		}
	}

	return true;
}

void EveProceduralMethodRandomParameter::SetModified( bool isModified )
{
	m_modified = isModified;
}

bool EveProceduralMethodRandomParameter::IsModified() const
{
	return m_modified;
}

int EveProceduralMethodRandomParameter::GetWeighting() const
{
	return m_weighting;
}

EveChildRefPtr EveProceduralMethodRandomParameter::GetChild()
{
	return m_child;
}

void EveProceduralMethodRandomParameter::Load()
{
	if( m_child != nullptr )
	{
		m_child->Reload( true );
	}
}