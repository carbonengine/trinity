// Copyright © 2013 CCP ehf.

#pragma once
#ifndef Tr2WindowHandle_h
#define Tr2WindowHandle_h

#ifdef _WIN32

typedef HWND Tr2WindowHandle;

#elif defined( __APPLE__ )

#include <objc/objc-runtime.h>
typedef id Tr2WindowHandle;

#else

typedef uintptr_t Tr2WindowHandle;

#endif


#endif // Tr2WindowHandle_h
