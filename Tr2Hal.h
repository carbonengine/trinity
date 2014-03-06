#pragma once
#ifndef Tr2Hal_h_
#define Tr2Hal_h_
 
#ifndef TRINITY_AL_WITH_BLUE_EXPOSURE
#define TRINITY_AL_WITH_BLUE_EXPOSURE 1
#endif

// --------------------------------------------------------
// - Platform macros
// - Do we have C++0x
// - Important constant and vertex buffer locations

#define TRINITY_DIRECTX9	1
#define TRINITY_DIRECTX11	2
#define TRINITY_OPENGLES2	3
#define TRINITY_ORBIS		4

#ifndef TRINITY_PLATFORM
#	error TRINITY_PLATFORM must be set
#endif

#if !defined( NDEBUG ) && !defined( TRINITY_AL_CAPTURE_ENABLED ) && ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )
#	define TRINITY_AL_CAPTURE_ENABLED		1
#endif


#ifdef _MSC_VER
#pragma warning(push, 3)
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
// for CComPtr support
#include <atlbase.h>

#include <D3D9.h>
#include <d3dx9.h>
#endif
#include <cstdint>
#include <algorithm>
#include <set>

#include "CcpCore/include/CcpCore.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// --------------------------------------------------------
// - Forward declare classes.
// - On some platforms Tr2PrimaryRenderContext is synonymous with
// Tr2RenderContextAL, others have it as a separate class.

class Tr2ConstantBufferAL;
class Tr2DepthStencilAL;
class Tr2IndexBufferAL;
class Tr2RenderContextAL;
class Tr2RenderTargetAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
class Tr2VertexBufferAL;
class Tr2VertexLayoutAL;
class Tr2GpuBufferAL;
class Tr2FenceAL;
class Tr2LockedRenderTargetAL;

#if( TRINITY_PLATFORM != TRINITY_DIRECTX11 )
#	define	Tr2PrimaryRenderContextAL		Tr2RenderContextAL
#else
class Tr2PrimaryRenderContextAL;
#endif

// --------------------------------------------------------
// - Including platform independent helpers and enums.

#include "ALResult.h"
#include "Tr2HalHelperStructures.h"
#include "Tr2HalFuzzing.h"
#include "Tr2RenderContextEnum.h"
#include "Tr2TrackedALObject.h"

#include "Tr2AdapterStructures.h"
#include "Tr2RenderCapture.h"

// --------------------------------------------------------
// - Start including actual implementations.  They all duck type
//    to the same interface.
// - Defines a platform name string
// - Defines the proper half texel offset for texture sampling.
// - Define macros for testing return codes, logging, and returning.

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#define TRINITY_PLATFORM_NAME "dx9"

const float AL_TEXEL_OFFSET = 0.5f;

#include <D3D9.h>

#include "Tr2VideoAdapterInfoALDx9.h"

#include "Tr2ConstantBufferALDx9.h"
#include "Tr2RenderContextDx9.h"

#include "Tr2VertexBufferALDx9.h"
#include "Tr2IndexBufferALDx9.h"
#include "Tr2VertexLayoutALDx9.h"
#include "Tr2SamplerStateALDx9.h"
#include "Tr2GpuBufferALDx9.h"

#include "Tr2DepthStencilALDx9.h"
#include "Tr2RenderTargetALDx9.h"
#include "Tr2SwapChainALDx9.h"

#include "Tr2ShaderALDx9.h"
#include "Tr2TextureALDx9.h"

#include "Tr2OcclusionQueryALDx9.h"
#include "Tr2FenceALDx9.h"
#include "Tr2LockedRenderTargetALDx9.h"

#elif( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#define TRINITY_PLATFORM_NAME "gles2"

const float AL_TEXEL_OFFSET = 0.0f;

#if defined(__ANDROID__)
#define TRINITY_AL_MOBILE
#endif

#ifdef TRINITY_AL_MOBILE
//#	include <EGL/egl.h>
//#	include <EGL/eglplatform.h>
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
//#	include <GLES2/gl2.h>
#endif

#include "Tr2VideoAdapterInfoALGLES2.h"

#include "Tr2VertexBufferALGLES2.h"
#include "Tr2IndexBufferALGLES2.h"
#include "Tr2ConstantBufferALGLES2.h"
#include "Tr2VertexLayoutALGLES2.h"
#include "Tr2SamplerStateALGLES2.h"
#include "Tr2GpuBufferALGLES2.h"

#include "Tr2DepthStencilALGLES2.h"
#include "Tr2RenderTargetALGLES2.h"

#include "Tr2SwapChainALGLES2.h"

#include "Tr2ShaderALGLES2.h"
#include "Tr2TextureALGLES2.h"

#include "Tr2OcclusionQueryALGLES2.h"
#include "Tr2FenceALGLES2.h"
#include "Tr2LockedRenderTargetALGLES2.h"



#include "Tr2RenderContextGLES2.h"


#elif( TRINITY_PLATFORM==TRINITY_ORBIS )

#define TRINITY_PLATFORM_NAME "orbis"

const float AL_TEXEL_OFFSET = 0.0f;

#include "Tr2VideoAdapterInfoALOrbis.h"

#include "Tr2VertexBufferALOrbis.h"
#include "Tr2IndexBufferALOrbis.h"
#include "Tr2ConstantBufferALOrbis.h"
#include "Tr2VertexLayoutALOrbis.h"
#include "Tr2ShaderALOrbis.h"
#include "Tr2TextureALOrbis.h"
#include "Tr2SamplerStateALOrbis.h"
#include "Tr2GpuBufferALOrbis.h"

#include "Tr2RenderTargetALOrbis.h"
#include "Tr2DepthStencilALOrbis.h"
#include "Tr2SwapChainALOrbis.h"

#include "Tr2RenderContextOrbis.h"

#include "Tr2OcclusionQueryALOrbis.h"
#include "Tr2FenceALOrbis.h"
#include "Tr2LockedRenderTargetALOrbis.h"

#elif( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#define TRINITY_PLATFORM_NAME "dx11"

const float AL_TEXEL_OFFSET = 0.0f;

#include <D3D11.h>
#include <D3DX11async.h>
#include <DXGI.h>

#include "Tr2VideoAdapterInfoALDx11.h"

#include "Tr2VertexBufferALDx11.h"
#include "Tr2IndexBufferALDx11.h"
#include "Tr2ConstantBufferALDx11.h"
#include "Tr2VertexLayoutALDx11.h"
#include "Tr2ShaderALDx11.h"
#include "Tr2TextureALDx11.h"
#include "Tr2SamplerStateALDx11.h"
#include "Tr2GpuBufferALDx11.h"

#include "Tr2RenderTargetALDx11.h"
#include "Tr2DepthStencilALDx11.h"
#include "Tr2SwapChainALDx11.h"

#include "Tr2RenderContextDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"

#include "Tr2OcclusionQueryALDx11.h"
#include "Tr2FenceALDx11.h"
#include "Tr2LockedRenderTargetALDx11.h"

#endif

#include "Tr2TrackedALObjectImpl.h"

#endif //Tr2Hal_h_
