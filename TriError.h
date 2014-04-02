/**************************************************************************************

	TriError.h

	Created:   October 2003
	OS:        Win32
	Project:   Trinity

	Description:   

		Tools for Direct3D error handling.

	Dependencies:

		Blue, DirectX 9.0

	(c) CCP 2003

**************************************************************************************/

#ifndef TRI_ERROR_H
#define TRI_ERROR_H

class TriError {
public:
	// Special Error reporting function for trinity objects, deciphers directX hresults, sets blue error state
	static void ReportError(HRESULT hr, Be::Clsid const *cid, const char* format, ...);

	//A version with a "pyerr" toggle.  Create an exception, or set blue error.
	static bool IsFailed( HRESULT hr, const Be::Clsid *cid, const char *format=0, ...);

#if BLUE_WITH_PYTHON
	// Used by the BLUE system to initialize the exception objects
	static void CreateExceptionObjects(PyObject *modDict);
#endif
};

#define ISFAILED(hr, _str, ...) TriError::IsFailed( hr, Clsid(), _str, __VA_ARGS__)
#define REPORTERRORDX(hr,_str, ...) TriError::ReportError( hr, Clsid(), _str, __VA_ARGS__ )
#define REPORTERROR(_str, ...) TriError::ReportError( BEDEF, Clsid(), _str, __VA_ARGS__ )

// macros to check results from COM interfaces, Like D3D. 
#define CHECKFAIL(_str) \
	do {if (FAILED(hr)){TriError::ReportError(hr, Clsid(), "%s", _str);return false;}}while(0)

// same as above, except it can sprintf one argument
#define CHECKFAIL2(_fmt, _arg) \
	do {if (FAILED(hr)){TriError::ReportError(hr, Clsid(), _fmt, _arg);return false;}}while(0)

// same as above, except it can sprintf two arguments
#define CHECKFAIL3(_fmt, _arg1, _arg2) \
	do {if (FAILED(hr)){TriError::ReportError(hr, Clsid(), _fmt, _arg1, _arg2);return false;}}while(0)


// macro to check whether a pointer is valid and return with error if none. 
#define CHECKPTR(_ptr, _str) \
	do {if (!_ptr){TriError::ReportError(BEDEF, Clsid(), _str);return false;}}while(0)

// Similar stuff for directX errors
#define CHECKDXFAIL(a) CHECKFAIL(a)
#define CHECKDXFAIL2(a,b) CHECKFAIL2(a,b)


#endif