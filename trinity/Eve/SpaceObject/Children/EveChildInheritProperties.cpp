// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildInheritProperties.h"

EveChildInheritProperties::EveChildInheritProperties( IRoot* lockobj ) :
	IRoot()
{
	memset( m_colorSet, 0, sizeof( m_colorSet ) );
}

EveChildInheritProperties::~EveChildInheritProperties()
{
}

void EveChildInheritProperties::SetProperties( const Color* colorSet )
{
	if( colorSet )
	{
		memcpy( m_colorSet, colorSet, sizeof( m_colorSet ) );
	}
}

const Color* EveChildInheritProperties::GetProperties()
{
	return m_colorSet;
}