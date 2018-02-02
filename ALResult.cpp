////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "ALResult.h"
#include "ALLog.h"

#if( TRINITYDEV == 1 )
bool g_requestDeviceDebugLayer = true;
#else
bool g_requestDeviceDebugLayer = false;
#endif


namespace
{

std::map<HRESULT, std::string> s_errorMessages;

}

#if defined(_DEBUG) || defined(TRINITYDEV)

void ReportHresultError( const char* fileName, int lineNumber, const char* statement, HRESULT hr )
{
	const char* msgFormat = "%s(%d) : '%s' returned error %s\n";
#ifdef HAS_DXERR
	const char* errorString = DXGetErrorStringA( hr );
#else
    char errorString[64];
	sprintf_s( errorString, "0x%X", hr );
#endif
	const int bufferSize = 1024;
	char buffer[ bufferSize ] = "";
	_snprintf_s( buffer, _TRUNCATE, msgFormat, fileName, lineNumber, statement, errorString );
#ifdef _WIN32
	OutputDebugString( buffer );
#else
	CCP_AL_LOGERR( "%s", buffer );
#endif
}

#if !defined( _WIN32 )
static void emptySignalHandler(int)
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
	__except(GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
	{
	}
#else
    struct sigaction action, oldAction;
    memset( &action, 0, sizeof( action ) );
    action.sa_handler = &emptySignalHandler;
    sigaction( SIGTRAP, &action, &oldAction );
    raise(SIGTRAP);
    sigaction( SIGTRAP, &oldAction, &action );
#endif
}

#endif

#if TRINITY_PLATFORM==TRINITY_OPENGLES2

void ReportGLError( const char* fileName, int lineNumber, const char* statement, unsigned errorCode )
{
	const char* msgFormat = "%s(%d) : '%s' returned error %s (%u)\n";
	const char* errorName;
	switch( errorCode )
	{
	case GL_INVALID_ENUM:
		errorName = "INVALID_ENUM";
		break;
	case GL_INVALID_VALUE:
		errorName = "INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		errorName = "INVALID_OPERATION";
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		errorName = "INVALID_FRAMEBUFFER_OPERATION";
		break;
	case GL_OUT_OF_MEMORY:
		errorName = "OUT_OF_MEMORY";
		break;
#if defined(_WIN32)
	case GL_STACK_UNDERFLOW:
		errorName = "STACK_UNDERFLOW";
		break;
	case GL_STACK_OVERFLOW:
		errorName = "STACK_OVERFLOW";
		break;
#endif
	default:
		errorName = "<unknown error>";
		break;
	}
	const int bufferSize = 1024;
	char buffer[ bufferSize ] = "";
	_snprintf_s( buffer, _TRUNCATE, msgFormat, fileName, lineNumber, statement, errorName, errorCode );
	CCP_AL_LOGERR( buffer );
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
template<> const char* BeGetErrorMessage( const Be::Result<HRESULT>& result )
{
	auto found = s_errorMessages.find( result );
	if( found == s_errorMessages.end() )
	{
#ifdef HAS_DXERR
		const char* name = DXGetErrorString( result );
		const char* message = DXGetErrorDescription( result );
#else
		const char* name = "<Unknown>";
		const char* message = "<No description>";
#endif
		static char buffer[1024];
		sprintf_s( buffer, "ALResult(%x) %s: %s", result.GetResult(), name, message );
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
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	case D3DERR_OUTOFVIDEOMEMORY:
#endif
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