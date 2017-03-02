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
#define TRINITY_STUB		5
#define TRINITY_OPENGL4		6

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
// for CComPtr support
#include <atlbase.h>
#if TRINITY_PLATFORM ==TRINITY_DIRECTX9 || TRINITY_PLATFORM ==TRINITY_DIRECTX11
#include <D3D9.h>
#include <d3dx9.h>
#endif
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
class Tr2GpuTimerAL;
class Tr2LockedRenderTargetAL;
class Tr2OcclusionQueryAL;
class Tr2SwapChainAL;

#if( TRINITY_PLATFORM != TRINITY_DIRECTX11 )
#	define	Tr2PrimaryRenderContextAL		Tr2RenderContextAL
#else
class Tr2PrimaryRenderContextAL;
#endif

// --------------------------------------------------------
// - Including platform independent helpers and enums.

#include "ALResult.h"
#include "Tr2HalHelperStructures.h"
#include "Tr2RenderContextEnum.h"
#include "Tr2TrackedALObject.h"

#include "Tr2AdapterStructures.h"

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

#include "dx9/Tr2VideoAdapterInfoALDx9.h"

#include "dx9/Tr2ConstantBufferALDx9.h"
#include "dx9/Tr2RenderContextDx9.h"

#include "dx9/Tr2VertexBufferALDx9.h"
#include "dx9/Tr2IndexBufferALDx9.h"
#include "dx9/Tr2VertexLayoutALDx9.h"
#include "dx9/Tr2SamplerStateALDx9.h"
#include "dx9/Tr2GpuBufferALDx9.h"

#include "dx9/Tr2DepthStencilALDx9.h"
#include "dx9/Tr2RenderTargetALDx9.h"
#include "dx9/Tr2SwapChainALDx9.h"

#include "dx9/Tr2ShaderALDx9.h"
#include "dx9/Tr2TextureALDx9.h"

#include "dx9/Tr2OcclusionQueryALDx9.h"
#include "dx9/Tr2FenceALDx9.h"
#include "dx9/Tr2GpuTimerALDx9.h"
#include "dx9/Tr2LockedRenderTargetALDx9.h"

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

#include "gles2/Tr2VideoAdapterInfoALGLES2.h"

#include "gles2/Tr2VertexBufferALGLES2.h"
#include "gles2/Tr2IndexBufferALGLES2.h"
#include "gles2/Tr2ConstantBufferALGLES2.h"
#include "gles2/Tr2VertexLayoutALGLES2.h"
#include "gles2/Tr2SamplerStateALGLES2.h"
#include "gles2/Tr2GpuBufferALGLES2.h"

#include "gles2/Tr2DepthStencilALGLES2.h"
#include "gles2/Tr2RenderTargetALGLES2.h"

#include "gles2/Tr2SwapChainALGLES2.h"

#include "gles2/Tr2ShaderALGLES2.h"
#include "gles2/Tr2TextureALGLES2.h"

#include "gles2/Tr2OcclusionQueryALGLES2.h"
#include "gles2/Tr2FenceALGLES2.h"
#include "gles2/Tr2GpuTimerALGLES2.h"
#include "gles2/Tr2LockedRenderTargetALGLES2.h"



#include "gles2/Tr2RenderContextGLES2.h"


#elif( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#define TRINITY_PLATFORM_NAME "dx11"

const float AL_TEXEL_OFFSET = 0.0f;

#include <D3D11.h>
#include <D3DX11async.h>
#include <DXGI.h>

#include "dx11/Tr2VideoAdapterInfoALDx11.h"

#include "dx11/Tr2VertexBufferALDx11.h"
#include "dx11/Tr2IndexBufferALDx11.h"
#include "dx11/Tr2ConstantBufferALDx11.h"
#include "dx11/Tr2VertexLayoutALDx11.h"
#include "dx11/Tr2ShaderALDx11.h"
#include "dx11/Tr2TextureALDx11.h"
#include "dx11/Tr2SamplerStateALDx11.h"
#include "dx11/Tr2GpuBufferALDx11.h"

#include "dx11/Tr2RenderTargetALDx11.h"
#include "dx11/Tr2DepthStencilALDx11.h"
#include "dx11/Tr2SwapChainALDx11.h"

#include "dx11/Tr2RenderContextDx11.h"
#include "dx11/Tr2PrimaryRenderContextDx11.h"

#include "dx11/Tr2OcclusionQueryALDx11.h"
#include "dx11/Tr2FenceALDx11.h"
#include "dx11/Tr2GpuTimerALDx11.h"
#include "dx11/Tr2LockedRenderTargetALDx11.h"

#elif( TRINITY_PLATFORM==TRINITY_STUB )

#define TRINITY_PLATFORM_NAME "dx11" // In order to use the dx11 platform specific res files as our own

const float AL_TEXEL_OFFSET = 0.0f;

#include "stub/Tr2VideoAdapterInfoALStub.h"

#include "stub/Tr2VertexBufferALStub.h"
#include "stub/Tr2IndexBufferALStub.h"
#include "stub/Tr2ConstantBufferALStub.h"
#include "stub/Tr2VertexLayoutALStub.h"
#include "stub/Tr2ShaderALStub.h"
#include "stub/Tr2TextureALStub.h"
#include "stub/Tr2SamplerStateALStub.h"
#include "stub/Tr2GpuBufferALStub.h"

#include "stub/Tr2RenderTargetALStub.h"
#include "stub/Tr2DepthStencilALStub.h"
#include "stub/Tr2SwapChainALStub.h"

#include "stub/Tr2RenderContextStub.h"

#include "stub/Tr2OcclusionQueryALStub.h"
#include "stub/Tr2FenceALStub.h"
#include "stub/Tr2GpuTimerALStub.h"
#include "stub/Tr2LockedRenderTargetALStub.h"

#elif( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#define GLFW_INCLUDE_GLCOREARB

#if defined(_WIN32)
#include <GL/glew.h>
#include <GL/gl.h>
#include "CL/cl.h"
#include "CL/cl_gl.h"

#elif defined(__APPLE__)
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3ext.h>
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl.h>
#include <OpenCL/opencl.h>
#else
#include <GL/glew.h>
#endif

#define TRINITY_PLATFORM_NAME "gl4"

const float AL_TEXEL_OFFSET = 0.0f;

#include "gl4/Tr2VideoAdapterInfoALGL4.h"

#include "gl4/Tr2VertexBufferALGL4.h"
#include "gl4/Tr2IndexBufferALGL4.h"
#include "gl4/Tr2ConstantBufferALGL4.h"
#include "gl4/Tr2VertexLayoutALGL4.h"
#include "gl4/Tr2ShaderALGL4.h"
#include "gl4/Tr2SamplerStateALGL4.h"
#include "gl4/Tr2TextureALGL4.h"
#include "gl4/Tr2GpuBufferALGL4.h"

#include "gl4/Tr2RenderTargetALGL4.h"
#include "gl4/Tr2DepthStencilALGL4.h"
#include "gl4/Tr2SwapChainALGL4.h"

#include "gl4/Tr2RenderContextGL4.h"

#include "gl4/Tr2OcclusionQueryALGL4.h"
#include "gl4/Tr2FenceALGL4.h"
#include "gl4/Tr2GpuTimerALGL4.h"
#include "gl4/Tr2LockedRenderTargetALGL4.h"

#endif

#include "Tr2TrackedALObjectImpl.h"
#include "Tr2GpuTelemetry.h"
#include "Tr2GpuTimerALContext.h"


#endif //Tr2Hal_h_
