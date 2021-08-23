#ifndef TrinityAL_StdAfx_H
#define TrinityAL_StdAfx_H

#ifdef _MSC_VER
#pragma warning(push, 3)
#endif

#ifdef _WIN32
#define NOMINMAX //don't want that evil microsoft macro
#include <windows.h> 
#endif

#include <string>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <memory>

#ifdef _WIN32
typedef HWND Tr2WindowHandle;
#elif defined(__APPLE__)
#include <objc/objc-runtime.h>
typedef id Tr2WindowHandle;
#else
typedef uintptr_t Tr2WindowHandle;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "include/TrinityALForward.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
#include <GFSDK_Aftermath.h>
#elif( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#if !defined(_WIN32) && !defined(TRINITY_AL_MOBILE)
#include "GLFW/glfw3.h"
#endif

#if defined(TRINITY_AL_MOBILE)
#include <android/native_window.h>
#endif

#endif

#endif
