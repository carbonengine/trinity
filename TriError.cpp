#include "StdAfx.h"
#include "TriError.h"
#include "TriPythonContext.h"

#ifdef _WIN32
#include <dxerr.h>
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
	const char *edsc = estr;
	if (hr != (HRESULT)BE32 && hr != (HRESULT)BEDEF)
    {
        estr = nullptr;
        edsc = nullptr;
#ifdef _WIN32
		estr = DXGetErrorString(hr);
		edsc = DXGetErrorDescription(hr);
#endif

		if (!estr)
			estr = "unknown error";
		if (!edsc)
			edsc = "unknown description";
	}
	sprintf_s(buff, "HRES:%X - %s(%s)\n", hr, estr, edsc);
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
		D3DERR_CONFLICTINGRENDERSTATE,
		D3DERR_CONFLICTINGTEXTUREFILTER,
		D3DERR_CONFLICTINGTEXTUREPALETTE,
		D3DERR_DEVICELOST,
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
		E_FAIL,
		E_INVALIDARG,
//		E_INVALIDCALL, //not found?
		E_NOINTERFACE,
		E_NOTIMPL,
		E_OUTOFMEMORY};

	for (int i = 0; i<sizeof(errors)/sizeof(errors[0]); i++) {
		HRESULT hr = errors[i];
		const char *estring = DXGetErrorString(hr);
		const char *edescription = DXGetErrorDescription(hr);
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
	t.Set(1, BluePyStr(DXGetErrorString(hr)?DXGetErrorString(hr):""));
	t.Set(2, BluePyStr(DXGetErrorDescription(hr)?DXGetErrorDescription(hr):""));
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
