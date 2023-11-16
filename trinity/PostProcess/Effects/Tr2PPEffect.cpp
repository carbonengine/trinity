////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPEffect.h"

Tr2PPEffect::Tr2PPEffect( IRoot* lockobj ) : 
	m_display(true), 
	m_isDirty(true)
{

}

Tr2PPEffect::~Tr2PPEffect( ) {

}

bool Tr2PPEffect::OnModified( Be::Var* value )
{
	m_isDirty = true;
	return true;
}

bool Tr2PPEffect::IsActive() {
	return m_display;
}

bool Tr2PPEffect::IsDirty() {
	return m_isDirty;
}

void Tr2PPEffect::SetDirty( bool dirty )
{
	m_isDirty = dirty;
}