// Copyright © 2023 CCP ehf.

#pragma once

#ifndef TRINITY_AL_WITH_BLUE_EXPOSURE
#define TRINITY_AL_WITH_BLUE_EXPOSURE 1
#endif

#define TRINITY_DIRECTX11 2
#define TRINITY_STUB 5
#define TRINITY_DIRECTX12 6
#define TRINITY_METAL 10

#ifndef TRINITY_PLATFORM
#error TRINITY_PLATFORM must be set
#endif


#ifdef _MSC_VER
#pragma warning( push, 3 )
#endif

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <atlbase.h>

#endif

#include <cstdint>
#include <algorithm>
#include <set>

#include <CcpCore.h>
#include <PixelFormat.h>
#include <CubemapFace.h>
#include <TextureType.h>
#include <BitmapDimensions.h>

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#if TRINITY_PLATFORM != TRINITY_DIRECTX11 && TRINITY_PLATFORM != TRINITY_DIRECTX12
#define TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT 0
#define Tr2PrimaryRenderContextAL Tr2RenderContextAL
#else
#define TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT 1
#endif

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#define TRINITY_PLATFORM_SYMBOL dx11
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Dx11
#define TRINITY_PLATFORM_NAME "dx11"

#include <D3D11.h>
#include <DXGI.h>

#elif ( TRINITY_PLATFORM == TRINITY_STUB )

#define TRINITY_PLATFORM_SYMBOL stub
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Stub
#define TRINITY_PLATFORM_NAME "dx11" // In order to use the dx11 platform specific res files as our own

#elif TRINITY_PLATFORM == TRINITY_DIRECTX12

#define TRINITY_PLATFORM_SYMBOL dx12
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Dx12
#define TRINITY_PLATFORM_NAME "dx12"

#include <d3d12.h>
#include <dxgi1_5.h>

#elif TRINITY_PLATFORM == TRINITY_METAL

#define TRINITY_PLATFORM_SYMBOL metal
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Metal
#define TRINITY_PLATFORM_NAME "metal"

#else

#error Missing TrinityAL platform description

#endif

// clang-format off
#define TRINITY_AL_PLATFORM_INCLUDE( className ) CCP_STRINGIZE(../TRINITY_PLATFORM_SYMBOL/CCP_CONCATENATE( className, TRINITY_PLATFORM_SYMBOL_SUFFIX ).h )
// clang-format on