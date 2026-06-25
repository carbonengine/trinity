// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "ALResult.h"
#include "ALLog.h"

#if ( TRINITYDEV == 1 )
bool g_requestDeviceDebugLayer = true;
#else
bool g_requestDeviceDebugLayer = false;
#endif
ICrashReporter* TrinityALCrashes = nullptr;


namespace
{

std::map<HRESULT, std::string> s_errorMessages;

}

#if defined( _DEBUG ) || defined( TRINITYDEV )

void ReportHresultError( const char* fileName, int lineNumber, const char* statement, HRESULT hr )
{
	const char* msgFormat = "%s(%d) : '%s' returned error 0x%X\n";
	const int bufferSize = 1024;
	char buffer[bufferSize] = "";
	_snprintf_s( buffer, _TRUNCATE, msgFormat, fileName, lineNumber, statement, hr );
#ifdef _WIN32
	OutputDebugString( buffer );
#else
	CCP_AL_LOGERR( "%s", buffer );
#endif
}

#if !defined( _WIN32 )
static void emptySignalHandler( int )
{
}
#endif

void BreakInDebugger()
{
#ifdef _WIN32
	__try
	{
		// This breakpoint exception is used by several D3D return value checking functions
		// If you get, here, go up the stack and see what D3D function failed
		DebugBreak();
	}
	__except( GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
	}
#else
	struct sigaction action, oldAction;
	memset( &action, 0, sizeof( action ) );
	action.sa_handler = &emptySignalHandler;
	sigaction( SIGTRAP, &action, &oldAction );
	raise( SIGTRAP );
	sigaction( SIGTRAP, &oldAction, &action );
#endif
}

#endif


// --------------------------------------------------------------------------------------
// Description:
//   Returns exception message for the given ALResult code.
// Arguments:
//   result - ALResult code
// Return value:
//   Static string with exception message
// --------------------------------------------------------------------------------------
template <>
const char* BeGetErrorMessage( const Be::Result<HRESULT>& result )
{
	auto found = s_errorMessages.find( result );
	if( found == s_errorMessages.end() )
	{
		static char buffer[1024];
		sprintf_s( buffer, "ALResult: %lx", result.GetResult() );
		s_errorMessages[result] = buffer;
		return s_errorMessages[result].c_str();
	}
	return found->second.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns ALResult error category.
// Return value:
//   ALResult error category
// --------------------------------------------------------------------------------------
Be::Result<HRESULT>::Category Be::Result<HRESULT>::GetCategory() const
{
	switch( GetResult() )
	{
	case E_OUTOFMEMORY:
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	case DXGI_ERROR_REMOTE_OUTOFMEMORY:
#endif
		return ALResult::OUT_OF_MEMORY;

	case E_DEVICELOST:
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	case DXGI_ERROR_DEVICE_HUNG:
	case DXGI_ERROR_DEVICE_REMOVED:
	case DXGI_ERROR_DEVICE_RESET:
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
#endif
		return ALResult::DEVICE_LOST;

	case E_INVALIDARG:
	case E_INVALIDCALL:
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	case DXGI_ERROR_INVALID_CALL:
#endif
		return ALResult::INVALID_CALL;

	default:
		if( SUCCEEDED( m_result ) )
		{
			return ALResult::SUCCESS;
		}
		else
		{
			return ALResult::OTHER;
		}
	}
}