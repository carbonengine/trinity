// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "TriStepClearUav.h"
#include "include/ITr2GpuBuffer.h"

// --------------------------------------------------------------------------------------
// Description:
//   TriStepClearUav default constructor
// --------------------------------------------------------------------------------------
TriStepClearUav::TriStepClearUav( IRoot* lockobj ) :
	m_clearWithFloat( false ),
	m_floatValue( 0.f, 0.f, 0.f, 0.f )
{
	std::fill( std::begin( m_uintValue ), std::end( m_uintValue ), 0 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements TriRenderStep. Clears writeable buffer with a set value.
// Arguments:
//   time - Current time
//   renderContext - Current render context
// Return Value:
//   RS_OK always
// --------------------------------------------------------------------------------------
TriStepResult TriStepClearUav::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_buffer )
	{
		auto buffer = m_buffer->GetGpuBuffer( 0 );
		if( buffer )
		{
			if( m_clearWithFloat )
			{
				CR( renderContext.ClearUav( *buffer, &m_floatValue.x ) );
			}
			else
			{
				CR( renderContext.ClearUav( *buffer, m_uintValue ) );
			}
		}
	}
	return RS_OK;
}
