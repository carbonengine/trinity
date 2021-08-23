#pragma once

#ifndef TRINITY_AL_WITH_BLUE_EXPOSURE
#define TRINITY_AL_WITH_BLUE_EXPOSURE 1
#endif

#define TRINITY_DIRECTX9	1
#define TRINITY_DIRECTX11	2
#define TRINITY_OPENGLES2	3
#define TRINITY_STUB		5
#define TRINITY_DIRECTX12	6
#define TRINITY_VULKAN      7
#define TRINITY_METAL       10

#ifndef TRINITY_PLATFORM
#	error TRINITY_PLATFORM must be set
#endif


#ifdef _MSC_VER
#pragma warning(push, 3)
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

#include "CcpCore/include/CcpCore.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if TRINITY_PLATFORM != TRINITY_DIRECTX11 && TRINITY_PLATFORM != TRINITY_DIRECTX12 && TRINITY_PLATFORM != TRINITY_VULKAN
#define TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT 0
#define	Tr2PrimaryRenderContextAL Tr2RenderContextAL
#else
#define TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT 1
#endif

#if TRINITY_PLATFORM==TRINITY_DIRECTX9 

#define TRINITY_PLATFORM_SYMBOL dx9
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Dx9
#define TRINITY_PLATFORM_NAME "dx9"
#include <D3D9.h>

#elif TRINITY_PLATFORM==TRINITY_DIRECTX11 

#define TRINITY_PLATFORM_SYMBOL dx11
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Dx11
#define TRINITY_PLATFORM_NAME "dx11"

#include <D3D11.h>
#include <DXGI.h>

#elif TRINITY_PLATFORM==TRINITY_OPENGLES2 

#define TRINITY_PLATFORM_SYMBOL gles2
#define TRINITY_PLATFORM_SYMBOL_SUFFIX GLES2
#define TRINITY_PLATFORM_NAME "gles2"
#ifdef TRINITY_AL_MOBILE
#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>

#define	GL_HALF_FLOAT		GL_HALF_FLOAT_OES
#define	GL_WRITE_ONLY		GL_WRITE_ONLY_OES
#define	GL_READ_ONLY		0

#define	glMapBuffer			glMapBufferOES
#define	glUnmapBuffer		glUnmapBufferOES

#elif defined(_WIN32)
#	include <GL/glew.h>
#	include <GL/gl.h>
#elif defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#else
#	include <GL/glew.h>
#endif

#if defined(TRINITY_AL_MOBILE)
#include <EGL/egl.h>
#endif

#elif( TRINITY_PLATFORM==TRINITY_STUB )

#define TRINITY_PLATFORM_SYMBOL stub
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Stub
#define TRINITY_PLATFORM_NAME "dx11" // In order to use the dx11 platform specific res files as our own

#elif TRINITY_PLATFORM == TRINITY_DIRECTX12

#define TRINITY_PLATFORM_SYMBOL dx12
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Dx12
#define TRINITY_PLATFORM_NAME "dx12"

#include <d3d12.h>
#include <dxgi1_4.h>

#elif TRINITY_PLATFORM == TRINITY_VULKAN

#define TRINITY_PLATFORM_SYMBOL vulkan
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Vulkan
#define TRINITY_PLATFORM_NAME "vulkan"

#pragma warning( push )
#pragma warning( disable: 4005 )
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#pragma warning( pop )

#elif TRINITY_PLATFORM == TRINITY_METAL

#define TRINITY_PLATFORM_SYMBOL metal
#define TRINITY_PLATFORM_SYMBOL_SUFFIX Metal
#define TRINITY_PLATFORM_NAME "metal"

#else

#error Missing TrinityAL platform description

#endif

#if TRINITY_PLATFORM == TRINITY_DIRECTX9 || TRINITY_PLATFORM == TRINITY_DIRECTX11
#define HAS_DXERR
#endif
#ifdef HAS_DXERR
#include <dxerr.h>
#endif


#define TRINITY_AL_PLATFORM_INCLUDE( className ) CCP_STRINGIZE(../TRINITY_PLATFORM_SYMBOL/CCP_CONCATENATE(className, TRINITY_PLATFORM_SYMBOL_SUFFIX).h)
