#ifndef Trinity_StdAfx_H
#define Trinity_StdAfx_H

// StdAfx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#define STRICT

// Note that we don't include <png.h> as that would get us the system installed libpng.h
// on Linux - we want the specific version we have in our sdk folder.
#include "png.h"
#if BLUE_WITH_PYTHON
#include <Python.h>
#endif

#include <stdint.h>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define NOMINMAX //don't want that evil microsoft macro
#include <windows.h> 

// for CComPtr support
#include <atlbase.h>
#include <direct.h>
#endif

#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <string>
#include <limits>
#include <stack>
#include <array>
#include <optional>
#include <float.h>
#include <math.h>
// TODO - remove using statements
using std::min;
using std::max;

// Disable identifier truncation warning
// #pragma warning( disable : 4786 )
#include <BlueExposure.h>
#include <BlueExposureMacrosDeprecated.h>
#include <IBlueOS.h>
#include <IBluePaths.h>
#include <IBlueResMan.h>
#include <IBluePersist.h>
#include <BlueStatistics.h>
#include <BlueAsyncRes.h>
#include <ICacheable.h>
#include <IBluePlacementObserver.h>
#include <IBlueEventListener.h>
#include <IBlueObjectProxy.h>
#include <BluePySwrap.h> //simple wrapping
#include <ITaskletTimer.h>

// here we define the directInput version we are going to use
// If this is not defined here manually there is a build warning
// which is very weird the dinput.h file should really define 
// this, as is done in D3D but I guess we'll just have to do it
#define DIRECTINPUT_VERSION 0x0800


#if !CCP_DEPLOY
#define NVPERFHUD 1
#endif

#ifdef _WIN32
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif


#include <CcpMath.h>

#include "include/Tr2WindowHandle.h"
#include <../trinityal/include/TrinityAL.h>
#include "Tr2RenderContext.h"
#include "Tr2RenderUtils.h"
#include "granny.h"
#include "TriUtil.h"

#if TBB_ENABLED

#include "tbb/parallel_sort.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_for_each.h"
#include "tbb/task.h"
#include "tbb/combinable.h"
#include "tbb/spin_mutex.h"

#endif

#if (_MSC_VER >= 1400 || _DLL)
#ifdef _HAS_EXCEPTIONS
#undef _HAS_EXCEPTIONS
#endif
#define _HAS_EXCEPTIONS 1
#endif

#ifndef EVALUATION_SDKS_ENABLED
	#define EVALUATION_SDKS_ENABLED 1
#endif

#ifndef GSTATE_ENABLED
#define GSTATE_ENABLED 1
#endif

#ifndef _WIN32
	#ifndef TRUE
		#define TRUE 1
	#endif
	#ifndef FALSE
		#define FALSE 0
	#endif

#endif

#include <Tr2ImageHandler.h>
#include <HostBitmap.h>


#include "Tr2GpuProfiler.h"

#define GPU_REGION( renderContext, label ) \
	GPU_REGION_AL( renderContext, label ); \
	Tr2GpuProfilerZone _profilerZone##__COUNTER__( nullptr, label, renderContext );


#endif