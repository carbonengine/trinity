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
#include <WinGDI.h>

#include <D3Dcompiler.h>
#include <d3d11.h>
#include <D3D9.h>
#include <D3DX9Effect.h>

#include <GL/glew.h>

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

#define D3DCOLORWRITEENABLE_RED     (1L<<0)
#define D3DCOLORWRITEENABLE_GREEN   (1L<<1)
#define D3DCOLORWRITEENABLE_BLUE    (1L<<2)
#define D3DCOLORWRITEENABLE_ALPHA   (1L<<3)

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

class ID3DXInclude
{};

typedef struct D3DXMACRO {
  LPCSTR Name;
  LPCSTR Definition;
} D3DXMACRO, *LPD3DXMACRO;

typedef enum D3DXINCLUDE_TYPE {
  D3DXINC_LOCAL        = 0,
  D3DXINC_SYSTEM       = 1,
  D3DXINC_FORCE_DWORD  = 0x7fffffff
} D3DXINCLUDE_TYPE, *LPD3DXINCLUDE_TYPE;

typedef struct D3DXFLOAT16 {
  WORD Value;
} D3DXFLOAT16, *LPD3DXFLOAT16;

typedef struct _D3DCOLORVALUE {

  float g;
  float b;
  float a;
} D3DCOLORVALUE;

typedef struct D3DXCOLOR
{
#ifdef __cplusplus
public:
    D3DXCOLOR() {}
    D3DXCOLOR( DWORD argb )
    {
        CONST FLOAT f = 1.0f / 255.0f;
        r = f * (FLOAT)(unsigned char)(argb >> 16);
        g = f * (FLOAT)(unsigned char)(argb >>  8);
        b = f * (FLOAT)(unsigned char)argb;
        a = f * (FLOAT)(unsigned char)(argb >> 24);
    }

#endif //__cplusplus
    FLOAT r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;

typedef enum D3DTEXTUREADDRESS {
  D3DTADDRESS_WRAP         = 1,
  D3DTADDRESS_MIRROR       = 2,
  D3DTADDRESS_CLAMP        = 3,
  D3DTADDRESS_BORDER       = 4,
  D3DTADDRESS_MIRRORONCE   = 5,
  D3DTADDRESS_FORCE_DWORD  = 0x7fffffff
} D3DTEXTUREADDRESS, *LPD3DTEXTUREADDRESS;

typedef enum D3DTEXTUREFILTERTYPE {
  D3DTEXF_NONE             = 0,
  D3DTEXF_POINT            = 1,
  D3DTEXF_LINEAR           = 2,
  D3DTEXF_ANISOTROPIC      = 3,
  D3DTEXF_PYRAMIDALQUAD    = 6,
  D3DTEXF_GAUSSIANQUAD     = 7,
  D3DTEXF_CONVOLUTIONMONO  = 8,
  D3DTEXF_FORCE_DWORD      = 0x7fffffff
} D3DTEXTUREFILTERTYPE, *LPD3DTEXTUREFILTERTYPE;

typedef enum D3DCMPFUNC {
  D3DCMP_NEVER         = 1,
  D3DCMP_LESS          = 2,
  D3DCMP_EQUAL         = 3,
  D3DCMP_LESSEQUAL     = 4,
  D3DCMP_GREATER       = 5,
  D3DCMP_NOTEQUAL      = 6,
  D3DCMP_GREATEREQUAL  = 7,
  D3DCMP_ALWAYS        = 8,
  D3DCMP_FORCE_DWORD   = 0x7fffffff
} D3DCMPFUNC, *LPD3DCMPFUNC;

typedef enum D3DBLEND {
  D3DBLEND_ZERO             = 1,
  D3DBLEND_ONE              = 2,
  D3DBLEND_SRCCOLOR         = 3,
  D3DBLEND_INVSRCCOLOR      = 4,
  D3DBLEND_SRCALPHA         = 5,
  D3DBLEND_INVSRCALPHA      = 6,
  D3DBLEND_DESTALPHA        = 7,
  D3DBLEND_INVDESTALPHA     = 8,
  D3DBLEND_DESTCOLOR        = 9,
  D3DBLEND_INVDESTCOLOR     = 10,
  D3DBLEND_SRCALPHASAT      = 11,
  D3DBLEND_BOTHSRCALPHA     = 12,
  D3DBLEND_BOTHINVSRCALPHA  = 13,
  D3DBLEND_BLENDFACTOR      = 14,
  D3DBLEND_INVBLENDFACTOR   = 15,
  D3DBLEND_SRCCOLOR2        = 16,
  D3DBLEND_INVSRCCOLOR2     = 17,
  D3DBLEND_FORCE_DWORD      = 0x7fffffff
} D3DBLEND, *LPD3DBLEND;

typedef enum D3DBLENDOP {
  D3DBLENDOP_ADD          = 1,
  D3DBLENDOP_SUBTRACT     = 2,
  D3DBLENDOP_REVSUBTRACT  = 3,
  D3DBLENDOP_MIN          = 4,
  D3DBLENDOP_MAX          = 5,
  D3DBLENDOP_FORCE_DWORD  = 0x7fffffff
} D3DBLENDOP, *LPD3DBLENDOP;

typedef enum D3DFILLMODE {
  D3DFILL_POINT        = 1,
  D3DFILL_WIREFRAME    = 2,
  D3DFILL_SOLID        = 3,
  D3DFILL_FORCE_DWORD  = 0x7fffffff
} D3DFILLMODE, *LPD3DFILLMODE;

typedef enum D3DCULL {
  D3DCULL_NONE         = 1,
  D3DCULL_CW           = 2,
  D3DCULL_CCW          = 3,
  D3DCULL_FORCE_DWORD  = 0x7fffffff
} D3DCULL, *LPD3DCULL;

typedef enum D3DRENDERSTATETYPE {
  D3DRS_ZENABLE                     = 7,
  D3DRS_FILLMODE                    = 8,
  D3DRS_SHADEMODE                   = 9,
  D3DRS_ZWRITEENABLE                = 14,
  D3DRS_ALPHATESTENABLE             = 15,
  D3DRS_LASTPIXEL                   = 16,
  D3DRS_SRCBLEND                    = 19,
  D3DRS_DESTBLEND                   = 20,
  D3DRS_CULLMODE                    = 22,
  D3DRS_ZFUNC                       = 23,
  D3DRS_ALPHAREF                    = 24,
  D3DRS_ALPHAFUNC                   = 25,
  D3DRS_DITHERENABLE                = 26,
  D3DRS_ALPHABLENDENABLE            = 27,
  D3DRS_FOGENABLE                   = 28,
  D3DRS_SPECULARENABLE              = 29,
  D3DRS_FOGCOLOR                    = 34,
  D3DRS_FOGTABLEMODE                = 35,
  D3DRS_FOGSTART                    = 36,
  D3DRS_FOGEND                      = 37,
  D3DRS_FOGDENSITY                  = 38,
  D3DRS_RANGEFOGENABLE              = 48,
  D3DRS_STENCILENABLE               = 52,
  D3DRS_STENCILFAIL                 = 53,
  D3DRS_STENCILZFAIL                = 54,
  D3DRS_STENCILPASS                 = 55,
  D3DRS_STENCILFUNC                 = 56,
  D3DRS_STENCILREF                  = 57,
  D3DRS_STENCILMASK                 = 58,
  D3DRS_STENCILWRITEMASK            = 59,
  D3DRS_TEXTUREFACTOR               = 60,
  D3DRS_WRAP0                       = 128,
  D3DRS_WRAP1                       = 129,
  D3DRS_WRAP2                       = 130,
  D3DRS_WRAP3                       = 131,
  D3DRS_WRAP4                       = 132,
  D3DRS_WRAP5                       = 133,
  D3DRS_WRAP6                       = 134,
  D3DRS_WRAP7                       = 135,
  D3DRS_CLIPPING                    = 136,
  D3DRS_LIGHTING                    = 137,
  D3DRS_AMBIENT                     = 139,
  D3DRS_FOGVERTEXMODE               = 140,
  D3DRS_COLORVERTEX                 = 141,
  D3DRS_LOCALVIEWER                 = 142,
  D3DRS_NORMALIZENORMALS            = 143,
  D3DRS_DIFFUSEMATERIALSOURCE       = 145,
  D3DRS_SPECULARMATERIALSOURCE      = 146,
  D3DRS_AMBIENTMATERIALSOURCE       = 147,
  D3DRS_EMISSIVEMATERIALSOURCE      = 148,
  D3DRS_VERTEXBLEND                 = 151,
  D3DRS_CLIPPLANEENABLE             = 152,
  D3DRS_POINTSIZE                   = 154,
  D3DRS_POINTSIZE_MIN               = 155,
  D3DRS_POINTSPRITEENABLE           = 156,
  D3DRS_POINTSCALEENABLE            = 157,
  D3DRS_POINTSCALE_A                = 158,
  D3DRS_POINTSCALE_B                = 159,
  D3DRS_POINTSCALE_C                = 160,
  D3DRS_MULTISAMPLEANTIALIAS        = 161,
  D3DRS_MULTISAMPLEMASK             = 162,
  D3DRS_PATCHEDGESTYLE              = 163,
  D3DRS_DEBUGMONITORTOKEN           = 165,
  D3DRS_POINTSIZE_MAX               = 166,
  D3DRS_INDEXEDVERTEXBLENDENABLE    = 167,
  D3DRS_COLORWRITEENABLE            = 168,
  D3DRS_TWEENFACTOR                 = 170,
  D3DRS_BLENDOP                     = 171,
  D3DRS_POSITIONDEGREE              = 172,
  D3DRS_NORMALDEGREE                = 173,
  D3DRS_SCISSORTESTENABLE           = 174,
  D3DRS_SLOPESCALEDEPTHBIAS         = 175,
  D3DRS_ANTIALIASEDLINEENABLE       = 176,
  D3DRS_MINTESSELLATIONLEVEL        = 178,
  D3DRS_MAXTESSELLATIONLEVEL        = 179,
  D3DRS_ADAPTIVETESS_X              = 180,
  D3DRS_ADAPTIVETESS_Y              = 181,
  D3DRS_ADAPTIVETESS_Z              = 182,
  D3DRS_ADAPTIVETESS_W              = 183,
  D3DRS_ENABLEADAPTIVETESSELLATION  = 184,
  D3DRS_TWOSIDEDSTENCILMODE         = 185,
  D3DRS_CCW_STENCILFAIL             = 186,
  D3DRS_CCW_STENCILZFAIL            = 187,
  D3DRS_CCW_STENCILPASS             = 188,
  D3DRS_CCW_STENCILFUNC             = 189,
  D3DRS_COLORWRITEENABLE1           = 190,
  D3DRS_COLORWRITEENABLE2           = 191,
  D3DRS_COLORWRITEENABLE3           = 192,
  D3DRS_BLENDFACTOR                 = 193,
  D3DRS_SRGBWRITEENABLE             = 194,
  D3DRS_DEPTHBIAS                   = 195,
  D3DRS_WRAP8                       = 198,
  D3DRS_WRAP9                       = 199,
  D3DRS_WRAP10                      = 200,
  D3DRS_WRAP11                      = 201,
  D3DRS_WRAP12                      = 202,
  D3DRS_WRAP13                      = 203,
  D3DRS_WRAP14                      = 204,
  D3DRS_WRAP15                      = 205,
  D3DRS_SEPARATEALPHABLENDENABLE    = 206,
  D3DRS_SRCBLENDALPHA               = 207,
  D3DRS_DESTBLENDALPHA              = 208,
  D3DRS_BLENDOPALPHA                = 209,
  D3DRS_FORCE_DWORD                 = 0x7fffffff
} D3DRENDERSTATETYPE, *LPD3DRENDERSTATETYPE;

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

#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <strstream>
#include <vector>
#include <regex>

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
