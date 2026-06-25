// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPythonCB.h"


CCP_STATS_DECLARE( rsPythonCBCount, "Trinity/RenderStep/PythonCBCount", true, CST_COUNTER_LOW, "Calls to TriStepPythonCB::Execute per frame" );

TriStepPythonCB::TriStepPythonCB( IRoot* lockobj )
{
}

TriStepPythonCB::~TriStepPythonCB()
{
}

void TriStepPythonCB::SetCallback( const BlueScriptCallback& callback )
{
	m_callback = callback;
}

TriStepResult TriStepPythonCB::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_STATS_INC( rsPythonCBCount );

	if( m_callback )
	{
		D3DPERF_EVENT( L"TriStepPythonCB callback" );

		if( !m_callback.CallVoid() )
		{
#if BLUE_WITH_PYTHON
			PyOS->PyFlushError( "TriStepPythonCB: Callback failed!" );
#endif
		}
	}

	return RS_OK;
}
