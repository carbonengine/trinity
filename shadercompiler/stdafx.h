// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif

#if _WIN32

#include <tchar.h>

#define NOMINMAX
#include <windows.h>
#include <atlbase.h>

#include <D3Dcompiler.h>
#include <d3d11.h>

#include <io.h>
#include <stdio.h>

#else

#include <cstdint>
#include <unistd.h>

// TODO MACOS: The definitions below are made to minimize the amount of code changes needed for ShaderCompiler
// at this stage.

#define __stdcall

#define _stricmp strcasecmp
#define _strnicmp strncasecmp

#define MAX_PATH 260

#define S_OK 0x00000000
#define E_FAIL 0x80004005

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)

#define D3D11_FILTER_REDUCTION_TYPE_MASK ( 0x3 )
#define D3D11_FILTER_REDUCTION_TYPE_SHIFT ( 7 )
#define D3D11_FILTER_TYPE_MASK ( 0x3 )
#define D3D11_MIN_FILTER_SHIFT ( 4 )
#define D3D11_MAG_FILTER_SHIFT ( 2 )
#define D3D11_MIP_FILTER_SHIFT ( 0 )
#define D3D11_ANISOTROPIC_FILTERING_BIT ( 0x40 )

#define D3D11_DECODE_MIN_FILTER( d3d11Filter )                                                              \
                                 ( ( D3D11_FILTER_TYPE )                                                    \
                                 ( ( ( d3d11Filter ) >> D3D11_MIN_FILTER_SHIFT ) & D3D11_FILTER_TYPE_MASK ) )
#define D3D11_DECODE_MAG_FILTER( d3d11Filter )                                                              \
                                 ( ( D3D11_FILTER_TYPE )                                                    \
                                 ( ( ( d3d11Filter ) >> D3D11_MAG_FILTER_SHIFT ) & D3D11_FILTER_TYPE_MASK ) )
#define D3D11_DECODE_MIP_FILTER( d3d11Filter )                                                              \
                                 ( ( D3D11_FILTER_TYPE )                                                    \
                                 ( ( ( d3d11Filter ) >> D3D11_MIP_FILTER_SHIFT ) & D3D11_FILTER_TYPE_MASK ) )
#define D3D11_DECODE_FILTER_REDUCTION( d3d11Filter )                                                        \
                                 ( ( D3D11_FILTER_REDUCTION_TYPE )                                                      \
                                 ( ( ( d3d11Filter ) >> D3D11_FILTER_REDUCTION_TYPE_SHIFT ) & D3D11_FILTER_REDUCTION_TYPE_MASK ) )
#define D3D11_DECODE_IS_COMPARISON_FILTER( d3d11Filter )                                                    \
                                 ( D3D11_DECODE_FILTER_REDUCTION( d3d11Filter ) == D3D11_FILTER_REDUCTION_TYPE_COMPARISON )
#define D3D11_DECODE_IS_ANISOTROPIC_FILTER( d3d11Filter )                                               \
                            ( ( ( d3d11Filter ) & D3D11_ANISOTROPIC_FILTERING_BIT ) &&                  \
                            ( D3D11_FILTER_TYPE_LINEAR == D3D11_DECODE_MIN_FILTER( d3d11Filter ) ) &&   \
                            ( D3D11_FILTER_TYPE_LINEAR == D3D11_DECODE_MAG_FILTER( d3d11Filter ) ) &&   \
                            ( D3D11_FILTER_TYPE_LINEAR == D3D11_DECODE_MIP_FILTER( d3d11Filter ) ) )

// Definitions taken from https://docs.microsoft.com/en-gb/windows/win32/winprog/windows-data-types
// Note: On Windows platform DWORD is defined as
// typedef unsigned long DWORD;
// and expected to be a 32-bit integer, but "long" is 64-bit on Mac.
// Same rationale applies for other integer types.
#define CONST const
typedef int BOOL;
typedef unsigned char BYTE;
// typedef unsigned long DWORD;
typedef uint32_t DWORD;
typedef float FLOAT;
// typedef unsigned int UINT;
typedef uint32_t UINT;
// typedef unsigned short WORD;
typedef uint16_t WORD;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef const void* LPCVOID;
typedef void* LPVOID;
typedef void *PVOID;
// typedef long LONG;
typedef int32_t LONG;
typedef LONG HRESULT;
typedef PVOID HANDLE;

typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

struct ID3DInclude
{
};

typedef struct D3D_SHADER_MACRO
{
  LPCSTR Name;
  LPCSTR Definition;
} D3D_SHADER_MACRO, *LPD3D_SHADER_MACRO;

enum D3D_INCLUDE_TYPE
{
	D3D_INCLUDE_LOCAL = 0,
	D3D_INCLUDE_SYSTEM = ( D3D_INCLUDE_LOCAL + 1 ),
	D3D_INCLUDE_FORCE_DWORD = 0x7fffffff
};

enum D3D11_FILTER {
	D3D11_FILTER_MIN_MAG_MIP_POINT = 0,
	D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x1,
	D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
	D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x5,
	D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,
	D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
	D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,
	D3D11_FILTER_MIN_MAG_MIP_LINEAR = 0x15,
	D3D11_FILTER_ANISOTROPIC = 0x55,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT = 0x80,
	D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
	D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
	D3D11_FILTER_COMPARISON_ANISOTROPIC = 0xd5,

    D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT = 0x100,
	D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
	D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
	D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
	D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
	D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
	D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
	D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
	D3D11_FILTER_MINIMUM_ANISOTROPIC = 0x155,
	D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
	D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
	D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
	D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
	D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
	D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
	D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
	D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
	D3D11_FILTER_MAXIMUM_ANISOTROPIC = 0x1d5
};

enum D3D11_COMPARISON_FUNC {
	D3D11_COMPARISON_NEVER = 1,
	D3D11_COMPARISON_LESS = 2,
	D3D11_COMPARISON_EQUAL = 3,
	D3D11_COMPARISON_LESS_EQUAL = 4,
	D3D11_COMPARISON_GREATER = 5,
	D3D11_COMPARISON_NOT_EQUAL = 6,
	D3D11_COMPARISON_GREATER_EQUAL = 7,
	D3D11_COMPARISON_ALWAYS = 8
};

enum D3D11_TEXTURE_ADDRESS_MODE {
	D3D11_TEXTURE_ADDRESS_WRAP = 1,
	D3D11_TEXTURE_ADDRESS_MIRROR = 2,
	D3D11_TEXTURE_ADDRESS_CLAMP = 3,
	D3D11_TEXTURE_ADDRESS_BORDER = 4,
	D3D11_TEXTURE_ADDRESS_MIRROR_ONCE = 5
};

enum D3D11_FILTER_TYPE {
  D3D11_FILTER_TYPE_POINT,
  D3D11_FILTER_TYPE_LINEAR
};

enum D3D11_FILTER_REDUCTION_TYPE {
  D3D11_FILTER_REDUCTION_TYPE_STANDARD,
  D3D11_FILTER_REDUCTION_TYPE_COMPARISON,
  D3D11_FILTER_REDUCTION_TYPE_MINIMUM,
  D3D11_FILTER_REDUCTION_TYPE_MAXIMUM
};

enum D3D11_BLEND
{
	D3D11_BLEND_ZERO = 1,
	D3D11_BLEND_ONE = 2,
	D3D11_BLEND_SRC_COLOR = 3,
	D3D11_BLEND_INV_SRC_COLOR = 4,
	D3D11_BLEND_SRC_ALPHA = 5,
	D3D11_BLEND_INV_SRC_ALPHA = 6,
	D3D11_BLEND_DEST_ALPHA = 7,
	D3D11_BLEND_INV_DEST_ALPHA = 8,
	D3D11_BLEND_DEST_COLOR = 9,
	D3D11_BLEND_INV_DEST_COLOR = 10,
	D3D11_BLEND_SRC_ALPHA_SAT = 11,
	D3D11_BLEND_BLEND_FACTOR = 14,
	D3D11_BLEND_INV_BLEND_FACTOR = 15,
	D3D11_BLEND_SRC1_COLOR = 16,
	D3D11_BLEND_INV_SRC1_COLOR = 17,
	D3D11_BLEND_SRC1_ALPHA = 18,
	D3D11_BLEND_INV_SRC1_ALPHA = 19
};

enum D3D11_BLEND_OP
{
	D3D11_BLEND_OP_ADD = 1,
	D3D11_BLEND_OP_SUBTRACT = 2,
	D3D11_BLEND_OP_REV_SUBTRACT = 3,
	D3D11_BLEND_OP_MIN = 4,
	D3D11_BLEND_OP_MAX = 5
};

enum D3D11_FILL_MODE
{
	D3D11_FILL_WIREFRAME = 2,
	D3D11_FILL_SOLID = 3
};

#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <strstream>
#include <vector>
#include <regex>

#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>
#include <thread>

#if CCP_TELEMETRY_ENABLED
#include "rad_tm.h"
#else
#define tmFunction( ... )
#define tmZone( ... )
#define tmThreadName( ... )

#endif

#ifndef _WIN32

inline errno_t fopen_s( FILE** stream, char const* fileName, char const* mode )
{
	*stream = fopen( fileName, mode );
	if( !*stream )
	{
		auto error = errno;
		return error ? error : -1;
	}
	return 0;
}

#endif
