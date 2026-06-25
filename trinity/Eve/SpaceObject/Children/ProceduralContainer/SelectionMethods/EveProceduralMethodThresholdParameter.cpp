// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodThresholdParameter.h"


EveProceduralMethodThresholdParameter::EveProceduralMethodThresholdParameter( IRoot* lockobj ) :
	m_threshold( 1.f ),
	m_modified( false )
{
}

EveProceduralMethodThresholdParameter::~EveProceduralMethodThresholdParameter()
{
}

bool EveProceduralMethodThresholdParameter::Initialize()
{
	if( m_child == nullptr )
	{
		m_child.CreateInstance();
	}
	return true;
}

bool EveProceduralMethodThresholdParameter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_threshold ) )
	{
		m_threshold = max( m_threshold, 0.f );
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

void EveProceduralMethodThresholdParameter::SetModified( bool isModified )
{
	m_modified = isModified;
}

bool EveProceduralMethodThresholdParameter::IsModified() const
{
	return m_modified;
}

float EveProceduralMethodThresholdParameter::GetThreshold() const
{
	return m_threshold;
}

EveChildRefPtr EveProceduralMethodThresholdParameter::GetChild()
{
	return m_child;
}

void EveProceduralMethodThresholdParameter::Load()
{
	if( m_child != nullptr )
	{
		m_child->Reload( true );
	}
}