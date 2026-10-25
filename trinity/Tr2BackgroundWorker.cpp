// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#include "Tr2BackgroundWorker.h"


namespace
{
IBlueCallbackManPtr s_instance;
}

IBlueCallbackMan& Tr2BackgroundWorker::Instance()
{
	if( !s_instance )
	{
		Be::Clsid clsid;
		if( BeClasses->FindClsid( clsid, "blue", "BlueCallbackMan" ) )
		{
			BeClasses->CreateInstance( clsid, BlueInterfaceIID<IBlueCallbackMan>(), (void**)&s_instance );
		}
		CCP_ASSERT( s_instance );

		s_instance->SetName( "Tr2BackgroundWorker" );
		s_instance->SetThreadCount( 1 );
		s_instance->SetPriority( -1 );
		s_instance->Run();
	}
	return *s_instance;
}
