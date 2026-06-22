// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveEllipseDefinition.h"

EveEllipseDefinition::EveEllipseDefinition( IRoot* )
{
}

bool EveEllipseDefinition::OnModified( Be::Var* )
{
	if( m_dirtyFlag )
	{
		*m_dirtyFlag = true;
	}
	return true;
}

void EveEllipseDefinition::SetDirtyFlag( bool* dirtyFlag )
{
	m_dirtyFlag = dirtyFlag;
}