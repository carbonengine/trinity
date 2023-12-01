// StdAfx.cpp : source file that includes just the standard includes
//	Io.pch will be the pre-compiled header
//	StdAfx.obj will contain the pre-compiled type information

#include "StdAfx.h"
#ifdef _WIN32

// throw -- terminate on thrown exception REPLACEABLE
#if !_HAS_EXCEPTIONS
#include <exception>
_STD_BEGIN

_CRTIMP2 _Prhand _Raise_handler = 0;	// define raise handler pointer as null

_CRTIMP2 void __cdecl _Throw(const exception& ex)
	{	// report error and die
		CCP_ASSERT(0);
	}
_STD_END
#endif
#endif
