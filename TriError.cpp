#include "StdAfx.h"
#include "TriError.h"
#include "TriPythonContext.h"

#ifdef _WIN32



namespace
{

#define  CHK_ERRA(hrchk) \
        case hrchk: \
             return #hrchk;


	const char* GetErrorString( HRESULT hr )
	{
		switch( hr )
		{
		CHK_ERRA( S_OK )
		CHK_ERRA( S_FALSE )

		CHK_ERRA( E_UNEXPECTED )
		CHK_ERRA( E_NOTIMPL )
		CHK_ERRA( E_OUTOFMEMORY )
		CHK_ERRA( E_INVALIDARG )
		CHK_ERRA( E_NOINTERFACE )
		CHK_ERRA( E_POINTER )
		CHK_ERRA( E_HANDLE )
		CHK_ERRA( E_ABORT )
		CHK_ERRA( E_FAIL )
		CHK_ERRA( E_ACCESSDENIED )
		CHK_ERRA( E_PENDING )
#if TRINITY_PLATFORM==TRINITY_DIRECTX11 || TRINITY_PLATFORM==TRINITY_DIRECTX12
		CHK_ERRA( DXGI_STATUS_OCCLUDED )
		CHK_ERRA( DXGI_STATUS_CLIPPED )
		CHK_ERRA( DXGI_STATUS_NO_REDIRECTION )
		CHK_ERRA( DXGI_STATUS_NO_DESKTOP_ACCESS )
		CHK_ERRA( DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE )
		CHK_ERRA( DXGI_STATUS_MODE_CHANGED )
		CHK_ERRA( DXGI_STATUS_MODE_CHANGE_IN_PROGRESS )
		CHK_ERRA( DXGI_ERROR_INVALID_CALL )
		CHK_ERRA( DXGI_ERROR_NOT_FOUND )
		CHK_ERRA( DXGI_ERROR_MORE_DATA )
		CHK_ERRA( DXGI_ERROR_UNSUPPORTED )
		CHK_ERRA( DXGI_ERROR_DEVICE_REMOVED )
		CHK_ERRA( DXGI_ERROR_DEVICE_HUNG )
		CHK_ERRA( DXGI_ERROR_DEVICE_RESET )
		CHK_ERRA( DXGI_ERROR_WAS_STILL_DRAWING )
		CHK_ERRA( DXGI_ERROR_FRAME_STATISTICS_DISJOINT )
		CHK_ERRA( DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE )
		CHK_ERRA( DXGI_ERROR_DRIVER_INTERNAL_ERROR )
		CHK_ERRA( DXGI_ERROR_NONEXCLUSIVE )
		CHK_ERRA( DXGI_ERROR_NOT_CURRENTLY_AVAILABLE )
		CHK_ERRA( DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED )
		CHK_ERRA( DXGI_ERROR_REMOTE_OUTOFMEMORY )

		CHK_ERRA( D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS )
		CHK_ERRA( D3D11_ERROR_FILE_NOT_FOUND )
		CHK_ERRA( D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS )
		CHK_ERRA( D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD )
#endif

		default: return "Unknown error.";
		}
	}

#undef CHK_ERRA

#define  CHK_ERR(hrchk, strOut) \
        case hrchk: \
             strcpy_s( desc, count, ##strOut ); break;


	void GetErrorDescription( HRESULT hr, char* desc, size_t count )
	{
		if( !count )
			return;

		*desc = 0;

		LPSTR errorText = nullptr;
		DWORD result = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, hr,
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&errorText, 0, nullptr );

		if( result > 0 && errorText )
		{
			strcpy_s( desc, count, errorText );

			if( errorText )
			{
				LocalFree( errorText );
			}
			return;
		}

#if TRINITY_PLATFORM==TRINITY_DIRECTX11 || TRINITY_PLATFORM==TRINITY_DIRECTX12
		switch( hr )
		{
		CHK_ERR( DXGI_STATUS_OCCLUDED, "The target window or output has been occluded. The application should suspend rendering operations if possible." )
		CHK_ERR( DXGI_STATUS_CLIPPED, "Target window is clipped." )
		CHK_ERR( DXGI_STATUS_NO_REDIRECTION, "" )
		CHK_ERR( DXGI_STATUS_NO_DESKTOP_ACCESS, "No access to desktop." )
		CHK_ERR( DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, "" )
		CHK_ERR( DXGI_STATUS_MODE_CHANGED, "Display mode has changed" )
		CHK_ERR( DXGI_STATUS_MODE_CHANGE_IN_PROGRESS, "Display mode is changing" )
		CHK_ERR( DXGI_ERROR_INVALID_CALL, "The application has made an erroneous API call that it had enough information to avoid. This error is intended to denote that the application should be altered to avoid the error. Use of the debug version of the DXGI.DLL will provide run-time debug output with further information." )
		CHK_ERR( DXGI_ERROR_NOT_FOUND, "The item requested was not found. For GetPrivateData calls, this means that the specified GUID had not been previously associated with the object." )
		CHK_ERR( DXGI_ERROR_MORE_DATA, "The specified size of the destination buffer is too small to hold the requested data." )
		CHK_ERR( DXGI_ERROR_UNSUPPORTED, "Unsupported." )
		CHK_ERR( DXGI_ERROR_DEVICE_REMOVED, "Hardware device removed." )
		CHK_ERR( DXGI_ERROR_DEVICE_HUNG, "Device hung due to badly formed commands." )
		CHK_ERR( DXGI_ERROR_DEVICE_RESET, "Device reset due to a badly formed commant." )
		CHK_ERR( DXGI_ERROR_WAS_STILL_DRAWING, "Was still drawing." )
		CHK_ERR( DXGI_ERROR_FRAME_STATISTICS_DISJOINT, "The requested functionality is not supported by the device or the driver." )
		CHK_ERR( DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE, "The requested functionality is not supported by the device or the driver." )
		CHK_ERR( DXGI_ERROR_DRIVER_INTERNAL_ERROR, "An internal driver error occurred." )
		CHK_ERR( DXGI_ERROR_NONEXCLUSIVE, "The application attempted to perform an operation on an DXGI output that is only legal after the output has been claimed for exclusive owenership." )
		CHK_ERR( DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "The requested functionality is not supported by the device or the driver." )
		CHK_ERR( DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED, "Remote desktop client disconnected." )
		CHK_ERR( DXGI_ERROR_REMOTE_OUTOFMEMORY, "Remote desktop client is out of memory." )

		CHK_ERR( D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS, "There are too many unique state objects." )
		CHK_ERR( D3D11_ERROR_FILE_NOT_FOUND, "File not found" )
		CHK_ERR( D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS, "Therea are too many unique view objects." )
		CHK_ERR( D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD, "Deferred context requires Map-Discard usage pattern" )
		default: strcpy_s( desc, count, "Unknown error." ); break;
		}
#else
		strcpy_s( desc, count, "Unknown error." );
#endif
	}

}


#endif

void ReportErrorV(
	HRESULT hr,
	Be::Clsid const *cid, 
	const char *format,
	va_list va)
{
	CCP_ASSERT(FAILED(hr));
	char buff[8192];
	const size_t bufsize = sizeof(buff);
	
	//Blue doesn't load the direxX dll and therefore doesn't have these functions
	const char *estr = "none";
	char edsc[1024];
	strcpy_s( edsc, estr );
	if (hr != (HRESULT)BE32 && hr != (HRESULT)BEDEF)
    {
#ifdef _WIN32
		estr = GetErrorString(hr);
		GetErrorDescription(hr, edsc, 1024);
#else
		estr = "unknown error";
		edsc = "unknown description";
#endif
	}
	sprintf_s(buff, "HRES:%X - %s(%s)\n", unsigned( hr ), estr, edsc);
	size_t idx = strlen(buff);
	if (format){		
		int wrote = vsnprintf_s(buff+idx, bufsize - idx, _TRUNCATE, format, va);
		if (wrote < -1) // some other error 
			BeOS->SetError(hr, cid, "Failed to format Error Info");		
	}
	BeOS->SetError(hr, cid, buff);
}

		
#if BLUE_WITH_PYTHON
//Exception objects
static BluePy d3dException;
typedef std::map<HRESULT, BluePy> d3dexceptions_t;
typedef d3dexceptions_t::iterator d3dexceptions_i;
static d3dexceptions_t d3dexceptions;

void TriError::CreateExceptionObjects(PyObject *modDict)
{
	d3dException = BluePy(PyErr_NewException(const_cast<char*>("trinity.D3DError"), PyExc_RuntimeError, 0));
	PyDict_SetItemString(modDict, "D3DError", d3dException);

#ifdef _WIN32
	HRESULT errors[] = {
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
		D3DERR_CONFLICTINGRENDERSTATE,
		D3DERR_CONFLICTINGTEXTUREFILTER,
		D3DERR_CONFLICTINGTEXTUREPALETTE,
		D3DERR_DEVICENOTRESET,
		D3DERR_DRIVERINTERNALERROR,
		D3DERR_DRIVERINVALIDCALL,
		D3DERR_INVALIDCALL,
		D3DERR_INVALIDDEVICE,
		D3DERR_MOREDATA,
		D3DERR_NOTAVAILABLE,
		D3DERR_NOTFOUND,
		D3DERR_OUTOFVIDEOMEMORY,
		D3DERR_TOOMANYOPERATIONS,
		D3DERR_UNSUPPORTEDALPHAARG,
		D3DERR_UNSUPPORTEDALPHAOPERATION,
		D3DERR_UNSUPPORTEDCOLORARG,
		D3DERR_UNSUPPORTEDCOLOROPERATION,
		D3DERR_UNSUPPORTEDFACTORVALUE,
		D3DERR_UNSUPPORTEDTEXTUREFILTER,
		D3DERR_WASSTILLDRAWING,
		D3DERR_WRONGTEXTUREFORMAT,
#endif
		E_DEVICELOST,
		E_FAIL,
		E_INVALIDARG,
//		E_INVALIDCALL, //not found?
		E_NOINTERFACE,
		E_NOTIMPL,
		E_OUTOFMEMORY};

	for (int i = 0; i<sizeof(errors)/sizeof(errors[0]); i++) {
		HRESULT hr = errors[i];
		const char *estring = GetErrorString(hr);
		char edescription[1024];
		GetErrorDescription( hr, edescription, 1024 );
		_ASSERT(estring && edescription);
		std::string name = estring;
		std::string fullname = "trinity." + name;
		BluePyStr help(edescription);
		BluePyDict dict(1);
		dict.Set("__doc__", help);

		BluePy ex(PyErr_NewException((char*)fullname.c_str(), d3dException, dict));
		PyDict_SetItemString(modDict, name.c_str(), ex);
		d3dexceptions.insert(std::pair<HRESULT, BluePy>(hr, ex));
	}
#else
    const char* errorNames[] = {
        "D3DERR_DEVICELOST",
        "D3DERR_NOTAVAILABLE",
        "D3DERR_OUTOFVIDEOMEMORY", };
    HRESULT errors[] = {
        E_DEVICELOST,
        (HRESULT)0x8876086A, // D3DERR_NOTAVAILABLE, not really issued, but is used by Python
        E_OUTOFMEMORY, };
	for (int i = 0; i<sizeof(errors)/sizeof(errors[0]); i++) {
		const char *estring = errorNames[i];
		const char *edescription = "";
		std::string name = estring;
		std::string fullname = "trinity." + name;
		BluePyStr help(edescription);
		BluePyDict dict(1);
		dict.Set("__doc__", help);
        
		BluePy ex(PyErr_NewException((char*)fullname.c_str(), d3dException, dict));
		PyDict_SetItemString(modDict, name.c_str(), ex);
		d3dexceptions.insert(std::pair<HRESULT, BluePy>(errors[i], ex));
	}
#endif
	//for backwards compatibility:
	PyDict_SetItemString(modDict, "DeviceLostError",
		                 d3dexceptions.find(E_DEVICELOST)->second);

	//Then the BEDEF error.
	BluePy ex(PyErr_NewException((char*)"trinity.BEDEF", PyExc_RuntimeError, 0));
	PyDict_SetItemString(modDict, "BEDEF", ex);
	d3dexceptions.insert(std::pair<HRESULT, BluePy>((HRESULT)BEDEF, ex));
}
#endif


void SetPyErrorV(HRESULT hr, const char *format, va_list va)
{
#if BLUE_WITH_PYTHON
	CCP_ASSERT(FAILED(hr));
	PyObject *ex;
	d3dexceptions_i i = d3dexceptions.find(hr);
	if (i!=d3dexceptions.end())
		ex = i->second;
	else
		ex = d3dException;
	
	BluePyStr msg;
	if (format) {
		msg = BluePyStr::FormatV(format, va);
	}
	BluePyTuple t(msg?4:3);
	t.Set(0, BluePyInt(hr));
#ifdef _WIN32
	t.Set( 1, BluePyStr( GetErrorString( hr ) ) );
	char desc[1024];
	GetErrorDescription( hr, desc, 1024 );
	t.Set(2, BluePyStr(desc));
#else
	t.Set(1, BluePyStr(""));
	t.Set(2, BluePyStr(""));
#endif
	if (msg)
		t.Set(3, msg);
	PyErr_SetObject(ex, t);
#endif
}

void TriError::ReportError(
	HRESULT error,
	Be::Clsid const *cid, 
	const char *format,
	...)
{
	va_list va;
	va_start(va, format);
	if( TriPythonContext::IsActive() )
	{
		SetPyErrorV( error, format, va );
	}
	else
	{
		ReportErrorV(error, cid, format, va);
	}
	va_end(va);
}


bool TriError::IsFailed( HRESULT hr, const Be::Clsid* cid, const char* format, ... )
 {
	if (!FAILED(hr))
		return false;
	va_list va;
	va_start(va, format);
	if ( TriPythonContext::IsActive() )
		SetPyErrorV(hr, format, va);
	else
		ReportErrorV(hr, cid, format, va);
	va_end(va);
	return true;
}
