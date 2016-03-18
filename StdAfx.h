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

#undef WINVER
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers


#ifdef _WIN32
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
#include <float.h>
#include <math.h>
// TODO - remove using statements
using std::min;
using std::max;

#ifndef TRINITYBUILD
#define TRINITYBUILD
#endif

// Disable identifier truncation warning
// #pragma warning( disable : 4786 )
#include "BlueExposure/include/BlueExposure.h"
#include "BlueExposure/include/BlueExposureMacrosDeprecated.h"
#include "blue/include/IBlueOS.h"
#include "blue/include/IBluePaths.h"
#include "blue/include/IBlueResMan.h"
#include "blue/include/IBluePersist.h"
#include "blue/include/BlueStatistics.h"
#include "Blue/include/BlueAsyncRes.h"
#include "blue/include/ICacheable.h"
#include "blue/include/IBluePlacementObserver.h"
#include <blue/include/TransGaming.h>
#include <blue/include/IBlueEventListener.h>
#include "blue/include/IBlueObjectProxy.h"
#include "blue/include/BluePySwrap.h" //simple wrapping
#include "Blue/Include/ITaskletTimer.h"

// here we define the directInput version we are going to use
// If this is not defined here manually there is a build warning
// which is very weird the dinput.h file should really define 
// this, as is done in D3D but I guess we'll just have to do it
#define DIRECTINPUT_VERSION 0x0800


#if !CCP_DEPLOY
#define NVPERFHUD 1
#endif

#ifndef APEX_ENABLED
    #ifdef _WIN32
	   #define APEX_ENABLED 1
    #else
        #define APEX_ENABLED 0
    #endif
#endif

#if APEX_ENABLED
#include "NxApex.h"
#endif

#ifdef _WIN32
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

#ifdef _WIN32
#include <d3dx9math.h>
#include <xnamath.h>
#else
#include "CcpMath/include/xnamath.h"
#include "CcpMath/include/Plane.h"
#include "CcpMath/include/Float16.h"
#endif

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Utilities/Matrix.h"
#include "Utilities/Color.h"
#include "Utilities/Quaternion.h"


#ifdef _WIN32
#include "D3DFundamentalTypes.h"
#endif

#include "include/Tr2WindowHandle.h"
#include "TrinityAL/Tr2Hal.h"
#include "Tr2RenderContext.h"
#include "Tr2RenderUtils.h"
#include "granny.h"
#include "TriUtil.h"

#if TBB_ENABLED

#include "tbb/parallel_sort.h"
#include "tbb/parallel_do.h"
#include "tbb/parallel_for.h"
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

#ifndef _WIN32
	#ifndef TRUE
		#define TRUE 1
	#endif
	#ifndef FALSE
		#define FALSE 0
	#endif

#endif

#if APEX_ENABLED

#include "NxApex.h"
#include "NxParameterized.h"
#include "NxParamUtils.h"
#include "NxClothingActor.h"
#include "PxAllocatorCallback.h"
#include "PxErrorCallback.h"
#include "NxCooking.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxPlaneShapeDesc.h"
#include "NxPlane.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "PhysXLoader.h"
#include "NxModuleClothing.h"
#include "NxDebugRenderable.h"
#include "NxUserRenderer.h"
#include "NxApexAsset.h"
#include "NxMat34.h"
#include "NxClothingAsset.h"

#endif

#include "ImageIO/Tr2ImageHandler.h"
#include "ImageIO/HostBitmap.h"

#endif