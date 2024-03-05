#include "StdAfx.h"
#include "TriStepUpdate.h"

TriStepUpdate::TriStepUpdate( IRoot* lockobj )
{
}

TriStepUpdate::~TriStepUpdate(void)
{
}

void TriStepUpdate::SetUpdateable( ITr2Updateable* obj )
{
	m_object = obj;
}

TriStepResult TriStepUpdate::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_object )
	{
		m_object->Update( realTime, simTime );
	}
	return RS_OK;
}