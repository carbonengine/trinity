// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodAttributeMapParameter.h"


EveProceduralMethodAttributeMapParameter::EveProceduralMethodAttributeMapParameter( IRoot* lockobj ) :
	m_modified( false )
{
}

EveProceduralMethodAttributeMapParameter::~EveProceduralMethodAttributeMapParameter()
{
}

bool EveProceduralMethodAttributeMapParameter::Initialize()
{
	if( m_child == nullptr )
	{
		m_child.CreateInstance();
	}
	return true;
}

bool EveProceduralMethodAttributeMapParameter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_child ) )
	{
		if( m_child != nullptr )
		{
			m_child->SetAutoLoadBlocker( true );
		}
	}

	return true;
}

void EveProceduralMethodAttributeMapParameter::SetModified( bool isModified )
{
	m_modified = isModified;
}

bool EveProceduralMethodAttributeMapParameter::IsModified() const
{
	return m_modified;
}

const char* EveProceduralMethodAttributeMapParameter::GetName() const
{
	return m_name.c_str();
}

EveChildRefPtr EveProceduralMethodAttributeMapParameter::GetChild()
{
	return m_child;
}

void EveProceduralMethodAttributeMapParameter::Load()
{
	if( m_child != nullptr )
	{
		m_child->Reload( true );
	}
}