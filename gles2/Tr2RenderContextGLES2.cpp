#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2RenderContextGLES2.h"
#include "ITr2RenderContextEvents.h"
#include "Tr2ConstantBufferALGLES2.h"
#include "Tr2ShaderALGLES2.h"
#include "Tr2VertexLayoutALGLES2.h"
#include "Tr2AdapterStructures.h"
#include "Tr2ShaderProgramALGLES2.h"
#include "Tr2ResourceSetALGLES2.h"
#include "Tr2BufferALGLES2.h"
#include "Tr2TextureALGLES2.h"

#include "ALLog.h"

using namespace Tr2RenderContextEnum;
#pragma warning( disable: 4189 )	// Scopeguard

bool g_gatherPipelineStatistics = false;

CCP_STATS_DECLARE( primitiveCount		, "Trinity/AL/primitiveCount"		, true, CST_COUNTER_HIGH, "Primitive count in DrawPrimitive calls." );
CCP_STATS_DECLARE( vertexCount			, "Trinity/AL/vertexCount"			, true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );
CCP_STATS_DECLARE( sceneDrawcallCount	, "Trinity/AL/sceneDrawcallCount"	, true, CST_COUNTER_LOW,  "Number of DrawPrimitive calls." );


#define INVALID_HRC (HGLRC)INVALID_HANDLE_VALUE

struct Tr2RenderContextAL::ShadowStateRestoreInfo
{
	GLenum m_instanceAttributes[16];
	unsigned m_instanceAttributesCount;
};

namespace
{

Tr2PrimaryRenderContextAL*& GetPrimaryRenderContextPointer()
{
	static Tr2PrimaryRenderContextAL* primaryRenderContext = nullptr;
	return primaryRenderContext;
}

#if defined(TRINITY_AL_MOBILE)

bool extensionsInitialized = false;

#define GL_TEXTURE_SRGB_DECODE_EXT 0x8A48
#define GL_DECODE_EXT 0x8A49
#define GL_SKIP_DECODE_EXT 0x8A4A
    
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
    
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
    
#define GL_SRGB_EXT 0x8C40
#define GL_SRGB_ALPHA_EXT 0x8C42
    
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9

typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDARBPROC) (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount);
PFNGLDRAWELEMENTSINSTANCEDARBPROC glDrawElementsInstancedARB = nullptr;
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBDIVISORARBPROC) (GLuint index, GLuint divisor);
PFNGLVERTEXATTRIBDIVISORARBPROC glVertexAttribDivisorARB = nullptr;

bool s_OES_vertex_half_float = false;
bool s_EXT_texture_sRGB_decode = false;
    
void InitializeExtensions()
{
    if( extensionsInitialized )
    {
        return;
    }
    extensionsInitialized = true;
    const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
    if( strstr( extensions, "GL_NV_instanced_arrays" ) || strstr( extensions, "GL_ANGLE_instanced_arrays" ) )
    {
        glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)eglGetProcAddress( "glDrawElementsInstancedNV" );
        if( !glDrawElementsInstancedARB )
        {
            glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)eglGetProcAddress( "glDrawElementsInstancedANGLE" );
        }
        glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)eglGetProcAddress( "glVertexAttribDivisorNV" );
        if( !glVertexAttribDivisorARB )
        {
            glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)eglGetProcAddress( "glVertexAttribDivisorANGLE" );
        }
    }
    s_OES_vertex_half_float = strstr( extensions, "OES_vertex_half_float" ) != 0;
    s_EXT_texture_sRGB_decode = strstr( extensions, "EXT_texture_sRGB_decode" ) != 0;
}
#endif

}

// --------------------------------------------------------------------------------------
// Description:
//   Internal helper class for blitting a texture into the frame buffer
// --------------------------------------------------------------------------------------
struct Tr2RenderContextAL::Blitter
{
	Blitter()
		:m_program( 0 )
	{
	}

	~Blitter()
	{
		if( m_program )
		{
			glDeleteProgram( m_program );
		}
	}
	// ----------------------------------------------------------------------------------
	// Description:
	//   Creates GPU objects needed for blitting.
	// Arguments:
	//   renderContext - Owning render context
	// Return value:
	//   AL result of the call
	// ----------------------------------------------------------------------------------
	ALResult Create( Tr2RenderContextAL& renderContext )
	{
		GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
		const char* vsSource = "attribute vec4 aPos;\n"
			"varying vec2 vTex;\n"
			"void main()\n"
			"{\n"
			"vTex.xy=aPos.zw;\n"
			"gl_Position=vec4(aPos.xy,0.0,1.0);\n"
			"}";
		GL_FAIL( glShaderSource( vertexShader, 1, &vsSource, nullptr ) );
		GL_FAIL( glCompileShader( vertexShader ) );
		GLint status = 0;
		GL_FAIL( glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &status ) );
		if( !status )
		{
			GLint length;
			glGetShaderiv( vertexShader, GL_INFO_LOG_LENGTH, &length );
			char* buffer = new char[length];
			glGetShaderInfoLog( vertexShader, length, nullptr, buffer );
			CCP_AL_LOGERR( "Tr2ShaderAL: error compiling shader: %s", buffer );
			delete[] buffer;
			return E_FAIL;
		}

		GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
		const char* fsSource =
#if defined(TRINITY_AL_MOBILE)
            "precision mediump float;\n"
#endif
            "varying vec2 vTex;\n"
			"uniform sampler2D tex;\n"
			"void main()\n"
			"{\n"
			"gl_FragColor=texture2D(tex, vTex);\n"
			"}";
		GL_FAIL( glShaderSource( fragmentShader, 1, &fsSource, nullptr ) );
		GL_FAIL( glCompileShader( fragmentShader ) );
		GL_FAIL( glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &status ) );
		if( !status )
		{
			GLint length;
			glGetShaderiv( vertexShader, GL_INFO_LOG_LENGTH, &length );
			char* buffer = new char[length];
			glGetShaderInfoLog( vertexShader, length, nullptr, buffer );
			CCP_AL_LOGERR( "Tr2ShaderAL: error compiling shader: %s", buffer );
			delete[] buffer;
			return E_FAIL;
		}

		m_program = glCreateProgram();
		GL_FAIL( glAttachShader( m_program, vertexShader ) );
		GL_FAIL( glAttachShader( m_program, fragmentShader ) );

		GL_FAIL( glLinkProgram( m_program ) );
		glGetProgramiv( m_program, GL_LINK_STATUS, &status );

		m_position = glGetAttribLocation( m_program, "aPos" );

		GL_FAIL( glUseProgram( m_program ) );

		GLint location = glGetUniformLocation( m_program, "tex" );
		GL_FAIL( glUniform1i( location, 0 ) );

		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );

		Tr2SamplerDescription desc( TF_POINT, TA_CLAMP );
		CR_RETURN_HR( m_sampler.Create( desc, renderContext ) );

		float vb[] = {
			-1.0f, -1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 0.0f,
		};
		CR_RETURN_HR( m_buffer.Create( sizeof( float ), 16, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vb, renderContext ) );
		return S_OK;
	}
	// --------------------------------------------------------------------------------------
	// Description:
	//   Blits (and flips vertically) a texture onto a current back buffer. Leaves GL state 
	//   dirty.
	// Arguments:
	//   source - Source texture to blit to the back buffer
	// Return value:
	//   AL result of the call
	// --------------------------------------------------------------------------------------
	ALResult Blit( Tr2TextureAL& source )
	{
		GL_FAIL( glActiveTexture( GL_TEXTURE0 ) );
		GL_FAIL( glBindTexture( GL_TEXTURE_2D, source.m_texture->m_texture ) );
		GL_IGNORE_ERROR( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT ) );
		if( !( source.m_texture->m_currentSampler.m_hash == m_sampler.m_sampler->m_stateData.m_hash ) )
		{
			CR_RETURN_HR( (HRESULT)m_sampler.m_sampler->Apply( GL_TEXTURE_2D, source.GetTrueMipCount() > 1, m_sampler.m_sampler->m_stateData ) );
			source.m_texture->m_currentSampler = m_sampler.m_sampler->m_stateData;
		}
		GL_FAIL( glUseProgram( m_program ) );

		GL_FAIL( glBindBuffer( GL_ARRAY_BUFFER, m_buffer.m_buffer->m_buffer ) );
		GL_FAIL( glEnableVertexAttribArray( m_position ) );
		GL_FAIL( glVertexAttribPointer( m_position,
								4,
								GL_FLOAT, 
								GL_FALSE,
								sizeof( float ) * 4, 
								(GLvoid*)0 ) );

		glDisable( GL_DEPTH_TEST );
		glDepthMask( GL_FALSE );
		glDisable( GL_CULL_FACE );
		glDisable( GL_BLEND );
		glDisable( GL_STENCIL_TEST );
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

		GL_FAIL( glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ) );
        
		return S_OK;
	}

	Tr2SamplerStateAL m_sampler;
	GLuint m_program;
	GLint m_position;
	Tr2BufferAL m_buffer;
};

Tr2RenderContextAL::Tr2RenderContextAL()
	: m_topology( TOP_TRIANGLES )
	, m_renderTargetHighWaterMark( 1 )
	, m_stackDS( "Tr2RenderContextAL::m_stackDS" )
	, m_boundIndexBufferIs16Bit( false )
	, m_boundLayout( nullptr )
#ifdef _WIN32
	, m_hRC( INVALID_HRC )
	, m_hDC( 0 )
#endif
	, m_hWnd( 0 )
	, m_defaultFrameBufferObject( 0 )
	, m_offscreenFrameBuffer0( 0 )
	, m_offscreenFrameBuffer1( 0 )
	, m_numberOfLights( 0 )
	, m_currentViewport( 0, 0 )
	, m_blitter( nullptr )
	, m_events( nullptr )
#if defined(TRINITY_AL_MOBILE)
    , m_display( EGL_NO_DISPLAY )
    , m_context( EGL_NO_CONTEXT )
    , m_surface( EGL_NO_SURFACE )
#endif
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() == nullptr );
	::GetPrimaryRenderContextPointer() = this;

	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_stackRT[i].SetName( "Tr2RenderContextAL::m_stackRT" );
	}

	m_blendState.separateAlphaBlend = false;
	m_blendState.blendMode = GL_FUNC_ADD;
	m_blendState.blendModeAlpha = GL_FUNC_ADD;
	m_blendState.srcBlend = GL_SRC_ALPHA;
	m_blendState.destBlend = GL_ONE_MINUS_SRC_ALPHA;
	m_blendState.srcBlendAlpha = GL_SRC_ALPHA;
	m_blendState.destBlendAlpha = GL_ONE_MINUS_SRC_ALPHA;
	m_blendState.dirty = true;

	m_stencilOpState.twoSided = false;
	m_stencilOpState.stencilFail = GL_KEEP;
	m_stencilOpState.stencilZFail = GL_KEEP;
	m_stencilOpState.stencilPass = GL_KEEP;
	m_stencilOpState.ccwStencilFail = GL_KEEP;
	m_stencilOpState.ccwStencilZFail = GL_KEEP;
	m_stencilOpState.ccwStencilPass = GL_KEEP;
	m_stencilOpState.dirty = true;

	m_stencilTestState.twoSided = false;
	m_stencilTestState.func = GL_ALWAYS;
	m_stencilTestState.ref = 0;
	m_stencilTestState.mask =  0xFFFFFFFF;
	m_stencilTestState.ccwFunc = GL_ALWAYS;
	m_stencilTestState.ccwRef = 0;
	m_stencilTestState.ccwMask =  0xFFFFFFFF;
	m_stencilTestState.dirty = true;

	m_depthOffsetState.slopeScaleOffset = 0.f;
	m_depthOffsetState.bias = 0.f;
	m_depthOffsetState.dirty = true;

	m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_alphaTestParameters.m_alphaTestRef = 0;
	m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;
	m_fragmentOpSettings.m_clipPlaneEnable = 0;
}

Tr2RenderContextAL::~Tr2RenderContextAL()
{
	Destroy();
}

void Tr2RenderContextAL::ReleaseOpenGLContext()
{
	if( m_defaultFrameBufferObject )
	{
		CR_GL( glDeleteFramebuffers( 1, &m_defaultFrameBufferObject ) );
		m_defaultFrameBufferObject = 0;
	}
	if( m_offscreenFrameBuffer0 )
	{
		CR_GL( glDeleteFramebuffers( 1, &m_offscreenFrameBuffer0 ) );
		m_offscreenFrameBuffer0 = 0;
	}
	if( m_offscreenFrameBuffer1 )
	{
		CR_GL( glDeleteFramebuffers( 1, &m_offscreenFrameBuffer1 ) );
		m_offscreenFrameBuffer1 = 0;
	}

	m_defaultBackBuffer = Tr2TextureAL();

#ifdef _WIN32
	if( m_hDC )
	{
		ReleaseDC( m_hWnd, m_hDC );
	}

	if( m_hRC != INVALID_HRC )
	{
		wglDeleteContext( m_hRC );
	}
	m_hRC  = INVALID_HRC;
#elif defined(TRINITY_AL_MOBILE)
    if( m_display != EGL_NO_DISPLAY )
    {
        eglMakeCurrent( m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        if( m_context != EGL_NO_CONTEXT )
        {
            eglDestroyContext( m_display, m_context );
            m_context = EGL_NO_CONTEXT;
        }
        if( m_surface != EGL_NO_SURFACE )
        {
            eglDestroySurface( m_display, m_surface );
            m_surface = EGL_NO_SURFACE;
        }
        eglTerminate( m_display );
        m_display = EGL_NO_DISPLAY;
    }
#endif
}

void Tr2RenderContextAL::SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* renderContext )
{
	::GetPrimaryRenderContextPointer() = renderContext;
}

Tr2PrimaryRenderContextAL& Tr2RenderContextAL::GetPrimaryRenderContext()
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() );
	return *GetPrimaryRenderContextPointer();
}

Tr2PrimaryRenderContextAL* Tr2RenderContextAL::GetPrimaryRenderContextPointer()
{
	return ::GetPrimaryRenderContextPointer();
}

void Tr2RenderContextAL::Destroy()
{
	ReleaseDeviceResources();
	m_samplerStateFactory.Clear();

	ReleaseOpenGLContext();

	m_topology = TOP_TRIANGLES;

	m_boundLayout = nullptr;
	m_program = Tr2ShaderProgramAL();

	for( unsigned i = 0; i < 8; ++i )
	{
		m_boundStreams[i].buffer = 0;
	}

	m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_alphaTestParameters.m_alphaTestRef = 0;
	m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;
	m_fragmentOpSettings.m_clipPlaneEnable = 0;

	m_hWnd = 0;
}

ALResult Tr2RenderContextAL::SetStreamSource(
	uint32_t stream,
	const Tr2BufferAL & buffer,
	uint32_t offset,
	uint32_t stride ) throw( )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	if( stream >= 8 )
	{
		return E_INVALIDARG;
	}
	if( !buffer.IsValid() )
	{
		m_boundStreams[stream].buffer = 0;
		m_boundStreams[stream].stride = 0;
		m_boundStreams[stream].offset = 0;
		return S_OK;
	}
	m_boundStreams[stream].buffer = buffer.m_buffer->m_buffer;
	m_boundStreams[stream].stride = stride;
	m_boundStreams[stream].offset = offset;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL & buffer ) throw( )
{
	m_boundIndexBufferIs16Bit = buffer.GetDesc().stride == 2;
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer.m_buffer->m_buffer );
	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL&, const float[4] ) throw( )
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL&, const uint32_t[4] ) throw( )
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::CopySubBuffer( Tr2BufferAL&, uint32_t, Tr2BufferAL&, uint32_t, uint32_t )
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::Clear(	
	uint32_t clearFlags, 
	uint32_t color, 
	float depth, 
	uint32_t stencil,
	uint32_t )
{
	if( !IsValid() )
	{ 
		return E_FAIL;
	}

	GLenum glFlags = 0;
	if( clearFlags & CLEARFLAGS_TARGET )
	{
		glFlags |= GL_COLOR_BUFFER_BIT;
		const float f = 1.0f / 255.0f;
		float r = f * (float) (uint8_t) (color >> 16);
		float g = f * (float) (uint8_t) (color >>  8);
		float b = f * (float) (uint8_t) (color >>  0);
		float a = f * (float) (uint8_t) (color >> 24);
		glClearColor( r, g, b, a );
	}
	uint32_t zWriteEnable = 0;
	if( clearFlags & CLEARFLAGS_ZBUFFER )
	{
		glFlags |= GL_DEPTH_BUFFER_BIT;
#if defined(TRINITY_AL_MOBILE)
		glClearDepthf( depth );
#else
		glClearDepth( depth );
#endif
		GetRenderState( RS_ZWRITEENABLE, &zWriteEnable );
		SetRenderState( RS_ZWRITEENABLE, 1 );
	}
	if( clearFlags & CLEARFLAGS_STENCIL )
	{
		glFlags |= GL_STENCIL_BUFFER_BIT;
		glClearStencil( stencil );
	}

	CR_GL( glClear( glFlags ) );

	if( clearFlags & CLEARFLAGS_ZBUFFER )
	{
		SetRenderState( RS_ZWRITEENABLE, zWriteEnable );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetTopology( long topology )
{
	m_topology = topology;
	return S_OK;
}

namespace {

	static GLenum lookup[ TOP_MAX_TOPOLOGY ] = 
	{
		GL_POINTS,		// invalid.. least likely to explode

		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,

		GL_LINES,
		GL_LINE_STRIP,

		GL_POINTS
	};

}

uint32_t Tr2RenderContextAL::ComputeVertexCount( uint32_t primitiveCount )
{
	switch( m_topology )
	{
	case TOP_TRIANGLES:
		return primitiveCount * 3;

	case TOP_TRIANGLE_STRIP:
		return primitiveCount + 2;

	case TOP_LINES:
		return primitiveCount * 2;

	case TOP_LINE_STRIP:
		return primitiveCount + 1;

	case TOP_POINTS:
	case TOP_TRIANGLE_FAN:
		return primitiveCount;
			
	
	default:
		CCP_ASSERT_M( false, "Unsupported topology" );
		return 0;
	}
}



ALResult Tr2RenderContextAL::DrawIndexedPrimitive(	
	uint32_t /*numVertices*/, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t /*minimumIndex*/ )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_INC( sceneDrawcallCount );

	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	ShadowStateRestoreInfo restore;
	if( !ApplyShadowRenderStates( restore ) || !ApplyVertexDeclaration( restore ) )
	{
		RestoreState( restore );
		return E_FAIL;
	}

	uintptr_t si = startIndex;

	CR_GL( glDrawElements( 
		lookup[ m_topology ], 
		ComputeVertexCount( primitiveCount ), 
		m_boundIndexBufferIs16Bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 
		(void*)(si * ( m_boundIndexBufferIs16Bit ? 2 : 4) ) ) );

	RestoreState( restore );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(	
	uint32_t /*numVertices*/, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t numInstances )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}
#if defined(TRINITY_AL_MOBILE)
    if( !glDrawElementsInstancedARB )
    {
        return E_FAIL;
    }
#endif
    
	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_INC( sceneDrawcallCount );

	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	ShadowStateRestoreInfo restore;
	if( !ApplyShadowRenderStates( restore ) || !ApplyVertexDeclaration( restore ) )
	{
		RestoreState( restore );
		return E_FAIL;
	}

	uintptr_t si = startIndex;
	CR_GL( glDrawElementsInstancedARB(
		lookup[ m_topology ],
		ComputeVertexCount( primitiveCount ),
		m_boundIndexBufferIs16Bit ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 
		(void*)(si * ( m_boundIndexBufferIs16Bit ? 2 : 4) ),
		numInstances ) );
	RestoreState( restore );
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	ShadowStateRestoreInfo restore;
	if( !ApplyShadowRenderStates( restore ) || !ApplyVertexDeclaration( restore ) )
	{
		RestoreState( restore );
		return E_FAIL;
	}

	CR_GL( glDrawArrays(	
		lookup[ m_topology ], 
		startVertex,
		ComputeVertexCount( primitiveCount ) ) );
	RestoreState( restore );
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP(		
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	ShadowStateRestoreInfo restore;
	if( !ApplyShadowRenderStates( restore ) || !ApplyVertexDeclaration( restore, vertexStreamZeroData, vertexStreamZeroStride ) )
	{
		RestoreState( restore );
		return E_FAIL;
	}

	CR_GL( glDrawArrays(	
		lookup[ m_topology ], 
		0,
		ComputeVertexCount( primitiveCount ) ) );
	RestoreState( restore );
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	ShadowStateRestoreInfo restore;
	if( !ApplyShadowRenderStates( restore ) || !ApplyVertexDeclaration( restore, vertexStreamZeroData, vertexStreamZeroStride ) )
	{
		RestoreState( restore );
		return E_FAIL;
	}

	GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );

	CR_GL( glDrawElements( 
		lookup[ m_topology ], 
		ComputeVertexCount( primitiveCount ), 
		GL_UNSIGNED_INT, 
		indexData ) );

	RestoreState( restore );
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	ShadowStateRestoreInfo restore;
    restore.m_instanceAttributesCount = 0;
	if( !ApplyShadowRenderStates( restore ) || !ApplyVertexDeclaration( restore, vertexStreamZeroData, vertexStreamZeroStride ) )
	{
		RestoreState( restore );
		return E_FAIL;
	}

	GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );

	CR_GL( glDrawElements( 
		lookup[ m_topology ], 
		ComputeVertexCount( primitiveCount ), 
		GL_UNSIGNED_SHORT, 
		indexData ) );

	RestoreState( restore );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants(	
	const Tr2ConstantBufferAL& buffer, 
	ShaderType constantType, 
	uint32_t registerIndex, 
	uint32_t /*maxRegisterCount*/ )
{
	if( registerIndex >= 16 )
	{
		return E_INVALIDARG;
	}

	if( constantType == PIXEL_SHADER && registerIndex == 0 )
	{
		registerIndex = CONSTANT_BUFFER_FOR_FRAGMENT_PARAMETERS;
	}

	m_boundBuffers[registerIndex] = buffer;

	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool /*enable*/ ) {}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return false;
}
ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2TextureAL& depthStencil )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	m_boundDepthStencil = depthStencil;
	
	return SetRtDsToDevice( MAX_RENDER_TARGET );
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}

	m_boundRenderTarget[slot] = renderTarget;
	m_renderTargetHighWaterMark = std::max( m_renderTargetHighWaterMark, slot+1 );

	return SetRtDsToDevice( slot );
}

namespace {

#ifdef _WIN32
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}
#endif

	auto checkGL = []
	{
		if( GLenum error = glGetError() )
		{
			CCP_AL_LOGERR(" GL error: %d", error );
		}
	};

	GLuint programObject;

}

ALResult Tr2RenderContextAL::CreateDevice(	
	uint32_t adapter, 
	Tr2WindowHandle  hFocusWindow, 
	const Tr2PresentParametersAL& presentationParameters )
{
    Tr2PresentParametersAL pp = presentationParameters;
    if( !pp.outputWindow )
    {
        pp.outputWindow = hFocusWindow;
    }
	CR_RETURN_HR( SetPresentParameters( adapter, pp ) );

	glEnable( GL_CULL_FACE );
	glEnable( GL_DEPTH_TEST );

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	return S_OK;
}

PixelFormat Tr2RenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer.GetFormat();
}

ALResult Tr2RenderContextAL::CreateOpenGLContext( Tr2PresentParametersAL& pp )
{
	m_hWnd = (Tr2WindowHandle)pp.outputWindow;
#ifdef _WIN32
	m_hDC = GetDC( m_hWnd );

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iFormat = ChoosePixelFormat( m_hDC, &pfd );
	SetPixelFormat( m_hDC, iFormat, &pfd );

	m_hRC = wglCreateContext( m_hDC );
	wglMakeCurrent( m_hDC, m_hRC );
#endif

#if !defined(TRINITY_AL_MOBILE)
	auto ret = glewInit();
	CCP_AL_LOGWARN( "glewInit %d", ret );
#else
	CR_RETURN_HR( Tr2VideoAdapterInfo::InternalInitializeEgl() );
    EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE };
    EGLConfig config;
    EGLint numConfigs;
    
    eglChooseConfig( display, attribs, &config, 1, &numConfigs );
    
    EGLint format;
    eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &format );
    ANativeWindow_setBuffersGeometry( reinterpret_cast<ANativeWindow*>( m_hWnd ), 0, 0, format );
    
    EGLSurface surface = eglCreateWindowSurface( display, config, reinterpret_cast<ANativeWindow*>( m_hWnd ), nullptr );
    
    EGLint contextAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE, };
    EGLContext context = eglCreateContext( display, config, nullptr, contextAttr );
    if( eglMakeCurrent( display, surface, surface, context ) == EGL_FALSE )
    {
        CCP_AL_LOGERR( "Tr2RenderContextAL: could not make current EGL context" );
    }
    
    EGLint w, h;
    eglQuerySurface( display, surface, EGL_WIDTH, &w );
    pp.mode.width = w;
    eglQuerySurface( display, surface, EGL_HEIGHT, &h );
    pp.mode.height = h;
    
    m_display = display;
    m_surface = surface;
    m_context = context;
    
    InitializeExtensions();
#endif

#if defined(TRINITY_AL_MOBILE)
    m_caps.m_supportsFloat16 = s_OES_vertex_half_float;
#else
	m_caps.m_supportsFloat16 = GLEW_ARB_half_float_vertex != 0;
#endif
    
	GL_FAIL( glGenFramebuffers( 1, &m_offscreenFrameBuffer0 ) );
	GL_FAIL( glGenFramebuffers( 1, &m_offscreenFrameBuffer1 ) );
	GL_FAIL( glGenFramebuffers( 1, &m_defaultFrameBufferObject ) );

	glFrontFace( GL_CCW );

	memset( m_renderStates, 0xff, sizeof( m_renderStates ) );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetPresentParameters( unsigned /*adapter*/, const Tr2PresentParametersAL& pp )
{
    Tr2PresentParametersAL presentationParameters = pp;
#ifdef _WIN32
	if( m_hRC == INVALID_HRC )
	{
		CR_RETURN_HR( CreateOpenGLContext( presentationParameters ) );
	}
	
	if( pp.windowed )
	{
		ChangeDisplaySettings( 0, CDS_NORESET );
	}
	else
	{
		DEVMODE dm;
		dm.dmSize = sizeof(DEVMODE);
		dm.dmPelsWidth = pp.mode.width;
		dm.dmPelsHeight = pp.mode.height;
		dm.dmBitsPerPel = 32;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
		ChangeDisplaySettings( &dm, CDS_FULLSCREEN );
	}

	typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>( wglGetProcAddress( "wglSwapIntervalEXT" ) );
	if( wglSwapIntervalEXT )
	{
		( *wglSwapIntervalEXT )( pp.presentInterval & 0xf );
	}
#else
    if( m_hWnd == 0 )
    {
        CR_RETURN_HR( CreateOpenGLContext( presentationParameters ) );
    }
#if !defined(TRINITY_AL_MOBILE)
    glfwSwapInterval( pp.presentInterval & 0xf );
#endif
#endif

	CR_RETURN_HR( m_defaultBackBuffer.Create(	
		Tr2BitmapDimensions( presentationParameters.mode.width, presentationParameters.mode.height, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ),
		Tr2GpuUsage::RENDER_TARGET,
		Tr2CpuUsage::READ,
		*this ) );

	SetRenderTarget( m_defaultBackBuffer );
	SetDepthStencil( Tr2TextureAL() );
	
	return S_OK;
}

const Tr2CapsAL& Tr2RenderContextAL::GetCaps() const
{
	return m_caps;
}

// --------------------------------------------------------------------------------------
// Description:
//   Blits specified texture to back buffer with flipping vertically. Used by ###.Present
//   methods. Not intended to be used outside AL.
// Arguments:
//   source - Source texture to blit to the back buffer
// Return value:
//   AL result of the call
// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::InternalBlitToBackBuffer( Tr2TextureAL& source )
{
	if( !IsValid() )
	{
		return E_INVALIDARG;
	}

	GL_FAIL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
	ON_BLOCK_EXIT( [&] { SetRtDsToDevice( 0 ); } );
	GL_FAIL( glViewport( 0, 0, source.GetWidth(), source.GetHeight() ) );

	if( !m_blitter )
	{
		m_blitter = new Blitter();
		CR_RETURN_HR( m_blitter->Create( *this ) );
	}
	ALResult result = m_blitter->Blit( source );

	m_program = Tr2ShaderProgramAL();
	if( m_renderStates[RS_ZENABLE] )
	{
		glEnable( GL_DEPTH_TEST );
	}
	if( m_renderStates[RS_ZWRITEENABLE] )
	{
		glDepthMask( GL_TRUE );
	}
	if( m_renderStates[RS_CULLMODE] != CULLMODE_NONE )
	{
		glEnable( GL_CULL_FACE );
	}
	if( m_renderStates[RS_ALPHABLENDENABLE] )
	{
		glEnable( GL_BLEND );
	}
	if( ( m_renderStates[RS_COLORWRITEENABLE] & 0xf ) != 0xf )
	{
		glColorMask( 
			m_renderStates[RS_COLORWRITEENABLE] & COLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
			m_renderStates[RS_COLORWRITEENABLE] & COLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
			m_renderStates[RS_COLORWRITEENABLE] & COLORWRITEENABLE_BLUE ? GL_TRUE : GL_FALSE,
			m_renderStates[RS_COLORWRITEENABLE] & COLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE );
	}
	return result;
}

ALResult Tr2RenderContextAL::Present()
{
	if( !IsValid() || !m_defaultBackBuffer.IsValid() )
	{
		return E_FAIL;
	}

	InternalBlitToBackBuffer( m_defaultBackBuffer );

#ifdef _WIN32
	if( !SwapBuffers( m_hDC ) )
	{
		return E_FAIL;
	}
#elif defined(TRINITY_AL_MOBILE)
    eglSwapBuffers( m_display, m_surface );
#else
    CR_GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
    glfwSwapBuffers( reinterpret_cast<GLFWwindow*>( m_hWnd ) );
    CR_GL( glBindFramebuffer( GL_FRAMEBUFFER, m_defaultFrameBufferObject ) );
#endif

	return S_OK;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout )
{
	if( !layout.IsValid() )
	{
		m_boundLayout = nullptr;
		return E_INVALIDARG;
	}
	m_boundLayout = layout.m_layout->m_definition.get();
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderProgram( const Tr2ShaderProgramAL& shaderProgram )
{
	m_program = shaderProgram;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetProgram()
{
	if( !m_program.IsValid() )
	{
		return E_FAIL;
	}

	bool patchedPS = (m_alphaTestParameters.m_alphaTestEnabled && 
		m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS) ||
		m_fragmentOpSettings.m_clipPlaneEnable;
	auto programData = patchedPS && m_program.m_program->m_patchedProgram.program ? &m_program.m_program->m_patchedProgram : &m_program.m_program->m_program;
	if( !programData->program )
	{
		return E_FAIL;
	}
	GL_FAIL( glUseProgram( programData->program ) );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count )
{
	for(	uint32_t i = 0; 
			( count != 0 && i != count ) || ( count == 0 && *stateValuePairs ); 
			++i, stateValuePairs += 2 )	
	{
		SetRenderState( static_cast<RenderState>( stateValuePairs[0] ), stateValuePairs[1] );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderState( RenderState state, uint32_t value )
{
	static const GLenum blendOps[] = {
		(GLenum)-1,						// --
		GL_ZERO,						// D3DBLEND_ZERO
		GL_ONE,							// D3DBLEND_ONE
		GL_SRC_COLOR,					// D3DBLEND_SRCCOLOR
		GL_ONE_MINUS_SRC_COLOR,			// D3DBLEND_INVSRCCOLOR
		GL_SRC_ALPHA,					// D3DBLEND_SRCALPHA
		GL_ONE_MINUS_SRC_ALPHA,			// D3DBLEND_INVSRCALPHA
		GL_DST_ALPHA,					// D3DBLEND_DESTALPHA
		GL_ONE_MINUS_DST_ALPHA,			// D3DBLEND_INVDESTALPHA
		GL_DST_COLOR,					// D3DBLEND_DESTCOLOR
		GL_ONE_MINUS_DST_COLOR,			// D3DBLEND_INVDESTCOLOR
		GL_SRC_ALPHA_SATURATE,			// D3DBLEND_SRCALPHASAT
		(GLenum)-1,						// D3DBLEND_BOTHSRCALPHA
		(GLenum)-1,						// D3DBLEND_BOTHINVSRCALPHA
		GL_CONSTANT_COLOR,				// D3DBLEND_BLENDFACTOR
		GL_ONE_MINUS_CONSTANT_COLOR,	// D3DBLEND_INVBLENDFACTOR
	};
	static const GLenum compareFuncs[] = {
		(GLenum)-1,		// --
		GL_NEVER,		// D3DCMP_NEVER
		GL_LESS,		// D3DCMP_LESS
		GL_EQUAL,		// D3DCMP_EQUAL
		GL_LEQUAL,		// D3DCMP_LESSEQUAL
		GL_GREATER,		// D3DCMP_GREATER
		GL_NOTEQUAL,	// D3DCMP_NOTEQUAL
		GL_GEQUAL,		// D3DCMP_GREATEREQUAL
		GL_ALWAYS,		// D3DCMP_ALWAYS
	};
	static const GLenum stencilOps[] = {
		(GLenum)-1,		// ---
		GL_KEEP,		// D3DSTENCILOP_KEEP
		GL_ZERO,		// D3DSTENCILOP_ZERO
		GL_REPLACE,		// D3DSTENCILOP_REPLACE
		GL_INCR,		// D3DSTENCILOP_INCRSAT
		GL_DECR,		// D3DSTENCILOP_DECRSAT
		GL_INVERT,		// D3DSTENCILOP_INVERT
		GL_INCR_WRAP,	// D3DSTENCILOP_INCR
		GL_DECR_WRAP,	// D3DSTENCILOP_DECR
	};
	static const GLenum blendFuncs[] = {
		(GLenum)-1,					// ---
		GL_FUNC_ADD,				// D3DBLENDOP_ADD
		GL_FUNC_SUBTRACT,			// D3DBLENDOP_SUBTRACT
		GL_FUNC_REVERSE_SUBTRACT,	// D3DBLENDOP_REVSUBTRACT
		(GLenum)-1,					// D3DBLENDOP_MIN
		(GLenum)-1,					// D3DBLENDOP_MAX
	};

	if( m_renderStates[state] == value )
	{
		return S_OK;
	}

	m_renderStates[state] = value;

	switch( state )
	{
	case RS_ZENABLE:
		if( value )
		{
			GL_FAIL( glEnable( GL_DEPTH_TEST ) );
		}
		else
		{
			GL_FAIL( glDisable( GL_DEPTH_TEST ) );
		}
		return S_OK;
	case RS_ZWRITEENABLE:
		GL_FAIL( glDepthMask( value ? GL_TRUE : GL_FALSE ) );
		return S_OK;
	case RS_ALPHATESTENABLE:
	case RS_ALPHAREF:
	case RS_ALPHAFUNC:
	case RS_CLIPPING:
	case RS_CLIPPLANEENABLE:
		m_fragmentOpSettings.SetRenderState( state, value, m_alphaTestParameters );
		return S_OK;
	case RS_SRCBLEND:
		if( value >= sizeof( blendOps ) / sizeof( GLenum ) || blendOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_blendState.dirty |= m_blendState.srcBlend != blendOps[value];
		m_blendState.srcBlend = blendOps[value];
		return S_OK;
	case RS_DESTBLEND:
		if( value >= sizeof( blendOps ) / sizeof( GLenum ) || blendOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_blendState.dirty |= m_blendState.destBlend != blendOps[value];
		m_blendState.destBlend = blendOps[value];
		return S_OK;
	case RS_CULLMODE:
		switch( value )
		{
		case CULLMODE_NONE:
			GL_FAIL( glDisable( GL_CULL_FACE ) );
			break;
		case CULLMODE_CW:
			GL_FAIL( glEnable( GL_CULL_FACE ) );
			GL_FAIL( glCullFace( GL_FRONT ) );
			break;
		case CULLMODE_CCW:
			GL_FAIL( glEnable( GL_CULL_FACE ) );
			GL_FAIL( glCullFace( GL_BACK ) );
			break;
		default:
			return E_FAIL;
		}
		return S_OK;
	case RS_ZFUNC:
		if( value >= sizeof( compareFuncs ) / sizeof( GLenum ) || compareFuncs[value] == 0 )
		{
			return E_FAIL;
		}
		GL_FAIL( glDepthFunc( compareFuncs[value] ) );
		return S_OK;
	case RS_ALPHABLENDENABLE:
		if( value )
		{
			GL_FAIL( glEnable( GL_BLEND ) );
		}
		else
		{
			GL_FAIL( glDisable( GL_BLEND ) );
		}
		return S_OK;
	case RS_STENCILENABLE:
		if( value )
		{
			GL_FAIL( glEnable( GL_STENCIL_TEST ) );
		}
		else
		{
			GL_FAIL( glDisable( GL_STENCIL_TEST ) );
		}
		return S_OK;
	case RS_STENCILFAIL:
		if( value >= sizeof( stencilOps ) / sizeof( GLenum ) || stencilOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilOpState.dirty |= m_stencilOpState.stencilFail != stencilOps[value];
		m_stencilOpState.stencilFail = stencilOps[value];
		return S_OK;
	case RS_STENCILZFAIL:
		if( value >= sizeof( stencilOps ) / sizeof( GLenum ) || stencilOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilOpState.dirty |= m_stencilOpState.stencilZFail != stencilOps[value];
		m_stencilOpState.stencilZFail = stencilOps[value];
		return S_OK;
	case RS_STENCILPASS:
		if( value >= sizeof( stencilOps ) / sizeof( GLenum ) || stencilOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilOpState.dirty |= m_stencilOpState.stencilPass != stencilOps[value];
		m_stencilOpState.stencilPass = stencilOps[value];
		return S_OK;
	case RS_STENCILFUNC:
		if( value >= sizeof( compareFuncs ) / sizeof( GLenum ) || compareFuncs[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilTestState.dirty |= m_stencilTestState.func != compareFuncs[value];
		m_stencilTestState.func = compareFuncs[value];
		return S_OK;
	case RS_STENCILREF:
		m_stencilTestState.dirty |= m_stencilTestState.ref != (GLint)value;
		m_stencilTestState.ref = value;
		return S_OK;
	case RS_STENCILMASK:
		m_stencilTestState.dirty |= m_stencilTestState.mask != value;
		m_stencilTestState.mask = value;
		return S_OK;
	case RS_STENCILWRITEMASK:
		GL_FAIL( glStencilMask( value ) );
		return S_OK;
	case RS_COLORWRITEENABLE:
		GL_FAIL( glColorMask( 
			value & COLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
			value & COLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
			value & COLORWRITEENABLE_BLUE ? GL_TRUE : GL_FALSE,
			value & COLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE ) );
		return S_OK;
	case RS_BLENDOP:
		if( value >= sizeof( blendFuncs ) / sizeof( GLenum ) || blendFuncs[value] == -1 )
		{
			return E_FAIL;
		}
		m_blendState.dirty |= m_blendState.blendMode != blendFuncs[value];
		m_blendState.blendMode = blendFuncs[value];
		return S_OK;
	case RS_SLOPESCALEDEPTHBIAS:
		m_depthOffsetState.dirty |= m_depthOffsetState.slopeScaleOffset != *(float*)&value;
		m_depthOffsetState.slopeScaleOffset = *(float*)&value;
		return S_OK;
	case RS_TWOSIDEDSTENCILMODE:
		m_stencilOpState.dirty |= m_stencilOpState.twoSided != ( value != 0 );
		m_stencilTestState.dirty |= m_stencilTestState.twoSided != ( value != 0 );
		m_stencilOpState.twoSided = value != 0;
		m_stencilTestState.twoSided = value != 0;
		return S_OK;
	case RS_CCW_STENCILFAIL:
		if( value >= sizeof( stencilOps ) / sizeof( GLenum ) || stencilOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilOpState.dirty |= m_stencilOpState.ccwStencilFail != stencilOps[value];
		m_stencilOpState.ccwStencilFail = stencilOps[value];
		return S_OK;
	case RS_CCW_STENCILZFAIL:
		if( value >= sizeof( stencilOps ) / sizeof( GLenum ) || stencilOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilOpState.dirty |= m_stencilOpState.ccwStencilZFail != stencilOps[value];
		m_stencilOpState.ccwStencilZFail = stencilOps[value];
		return S_OK;
	case RS_CCW_STENCILPASS:
		if( value >= sizeof( stencilOps ) / sizeof( GLenum ) || stencilOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilOpState.dirty |= m_stencilOpState.ccwStencilPass != stencilOps[value];
		m_stencilOpState.ccwStencilPass = stencilOps[value];
		return S_OK;
	case RS_CCW_STENCILFUNC:
		if( value >= sizeof( compareFuncs ) / sizeof( GLenum ) || compareFuncs[value] == -1 )
		{
			return E_FAIL;
		}
		m_stencilTestState.dirty |= m_stencilTestState.ccwFunc != compareFuncs[value];
		m_stencilTestState.ccwFunc = compareFuncs[value];
		return S_OK;
	case RS_BLENDFACTOR:
		GL_FAIL( glBlendColor( 
			float( value & 0xff ) / 255.f,
			float( ( value >> 8 ) & 0xff ) / 255.f,
			float( ( value >> 16 ) & 0xff ) / 255.f,
			float( ( value >> 24 ) & 0xff ) / 255.f ) );
		return S_OK;
	case RS_DEPTHBIAS:
	case RS_ZBIAS:
		m_depthOffsetState.dirty |= m_depthOffsetState.bias != *(float*)&value;
		m_depthOffsetState.bias = *(float*)&value;
		return S_OK;
	case RS_SEPARATEALPHABLENDENABLE:
		m_blendState.dirty |= m_blendState.separateAlphaBlend != ( value != 0 );
		m_blendState.separateAlphaBlend = value != 0;
		return S_OK;
	case RS_BLENDOPALPHA:
		m_blendState.dirty |= m_blendState.blendModeAlpha != blendFuncs[value];
		m_blendState.blendModeAlpha = blendFuncs[value];
		return S_OK;
	case RS_SRCBLENDALPHA:
		if( value >= sizeof( blendOps ) / sizeof( GLenum ) || blendOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_blendState.dirty |= m_blendState.srcBlendAlpha != blendOps[value];
		m_blendState.srcBlendAlpha = blendOps[value];
		return S_OK;
	case RS_DESTBLENDALPHA:
		if( value >= sizeof( blendOps ) / sizeof( GLenum ) || blendOps[value] == -1 )
		{
			return E_FAIL;
		}
		m_blendState.dirty |= m_blendState.destBlendAlpha != blendOps[value];
		m_blendState.destBlendAlpha = blendOps[value];
		return S_OK;
	case RS_SRGBWRITEENABLE:
		if( value )
		{
			GL_FAIL( glEnable( GL_FRAMEBUFFER_SRGB_EXT ) );
		}
		else
		{
			GL_FAIL( glDisable( GL_FRAMEBUFFER_SRGB_EXT ) );
		}
		return S_OK;
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2RenderContextAL::GetRenderState( RenderState state, uint32_t* value )
{
	*value = m_renderStates[state];
	return S_OK;
}

ALResult Tr2RenderContextAL::SetResourceSet( const Tr2ResourceSetAL& resourceSet )
{
	if( !resourceSet.IsValid() )
	{
		return E_INVALIDARG;
	}

	for( auto it = resourceSet.m_resourceSet->m_textures.begin(); it != resourceSet.m_resourceSet->m_textures.end(); ++it )
	{
		if( !it->texture )
		{
			continue;
		}
		GL_FAIL( glActiveTexture( GL_TEXTURE0 + it->registerIndex ) );
		GL_FAIL( glBindTexture( it->type, it->texture ) );
		GL_IGNORE_ERROR( glTexParameteri( it->type, GL_TEXTURE_SRGB_DECODE_EXT, it->srgbDecode ) );
		TrinityALImpl::Tr2SamplerStateAL::Apply( it->type, it->hasMips, it->sampler );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetNumberOfLights( uint32_t numLights )
{
	m_numberOfLights = numLights;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport )
{
	m_currentViewport = viewport;
	CR_GL( glViewport(	GLint( viewport.m_x ), 
						GLint( viewport.m_y ), 
						GLint( viewport.m_width ), 
						GLint( viewport.m_height ) ) );
#if defined(TRINITY_AL_MOBILE)
	CR_GL( glDepthRangef( viewport.m_minZ, viewport.m_maxZ ) );
#else
	CR_GL( glDepthRange( viewport.m_minZ, viewport.m_maxZ ) );
#endif
	return S_OK;
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport )
{
	viewport = m_currentViewport;
	return S_OK;
}

ALResult Tr2RenderContextAL::PushRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	
	m_stackRT[slot].push( m_boundRenderTarget[slot] );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	CCP_ASSERT( !m_stackRT[slot].empty() );
	if( m_stackRT[slot].empty() )
	{
		return E_FAIL;
	}

	m_boundRenderTarget[slot] = m_stackRT[slot].top();
	m_stackRT[slot].pop();
	return SetRtDsToDevice( slot );
}
	
ALResult Tr2RenderContextAL::PushDepthStencil()
{
	m_stackDS.push( m_boundDepthStencil );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopDepthStencil()
{
	CCP_ASSERT( !m_stackDS.empty() );
	if( m_stackDS.empty() )
	{
		return E_INVALIDARG;
	}

	m_boundDepthStencil = m_stackDS.top();
	m_stackDS.pop();
	return SetRtDsToDevice( MAX_RENDER_TARGET );
}

ALResult Tr2RenderContextAL::GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	if( m_boundRenderTarget[slot].IsValid() )
	{
		width = m_boundRenderTarget[slot].GetWidth();
		height = m_boundRenderTarget[slot].GetHeight();
	}
	else if( slot >= 1 )
	{
		return E_FAIL;
	}
	else
	{
		width = m_defaultBackBuffer.GetWidth();
		height = m_defaultBackBuffer.GetHeight();
	}
	return S_OK;
}

bool Tr2RenderContextAL::ConvertToGLPixelFormat(	PixelFormat format, 
												GLenum& internalFormat, 
												GLenum& targetFormat, 
												GLenum& targetType )
{
	switch( format )
	{
#if defined(TRINITY_AL_MOBILE)
        
    case PIXEL_FORMAT_A8_UNORM:
        internalFormat = s_EXT_texture_sRGB_decode ? GL_SRGB_ALPHA_EXT : GL_RGBA;
        targetFormat = GL_ALPHA;
        targetType = GL_UNSIGNED_BYTE;
        return true;
        
    case PIXEL_FORMAT_BC1_TYPELESS:
    case PIXEL_FORMAT_BC1_UNORM:
    case PIXEL_FORMAT_BC1_UNORM_SRGB:
        internalFormat = s_EXT_texture_sRGB_decode ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        targetFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        targetType   = 0;
        return true;
        
    case PIXEL_FORMAT_BC2_TYPELESS:
    case PIXEL_FORMAT_BC2_UNORM:
    case PIXEL_FORMAT_BC2_UNORM_SRGB:
        internalFormat = s_EXT_texture_sRGB_decode ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        targetFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        targetType   = 0;
        return true;
        
    case PIXEL_FORMAT_BC3_TYPELESS:
    case PIXEL_FORMAT_BC3_UNORM:
    case PIXEL_FORMAT_BC3_UNORM_SRGB:
        internalFormat = s_EXT_texture_sRGB_decode ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        targetFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        targetType   = 0;
        return true;
        
    case PIXEL_FORMAT_B5G6R5_UNORM:
        internalFormat = GL_RGB;
        targetFormat = GL_RGB;
        targetType   = GL_UNSIGNED_SHORT_5_6_5;
        return true;
        
    case PIXEL_FORMAT_B5G5R5A1_UNORM:
        internalFormat = GL_RGBA;
        targetFormat = GL_RGBA;
        targetType   = GL_UNSIGNED_SHORT_5_5_5_1;
        return true;
        
    case PIXEL_FORMAT_B8G8R8A8_UNORM:
    case PIXEL_FORMAT_B8G8R8A8_TYPELESS:
    case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
        internalFormat = s_EXT_texture_sRGB_decode ? GL_SRGB_ALPHA_EXT : GL_BGRA_EXT;
        targetFormat = GL_BGRA_EXT;
        targetType   = GL_UNSIGNED_BYTE;
        return true;
        
    case PIXEL_FORMAT_B8G8R8X8_UNORM:
    case PIXEL_FORMAT_B8G8R8X8_TYPELESS:
    case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
        internalFormat = GL_SRGB_EXT;
        targetFormat = GL_BGRA_EXT;
        targetType   = GL_UNSIGNED_BYTE;
        return true;
#else
	case PIXEL_FORMAT_R32G32B32A32_TYPELESS:
	case PIXEL_FORMAT_R32G32B32A32_FLOAT:
		internalFormat = GL_RGBA32F;
		targetFormat = GL_RGBA;
		targetType   = GL_FLOAT;
		return true;

	case PIXEL_FORMAT_R32G32B32A32_UINT:
		internalFormat = GL_RGBA32UI;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_INT;
		return true;

	case PIXEL_FORMAT_R32G32B32A32_SINT:
		internalFormat = GL_RGBA32I;
		targetFormat = GL_RGBA;
		targetType   = GL_INT;
		return true;

	case PIXEL_FORMAT_R32G32B32_TYPELESS:
	case PIXEL_FORMAT_R32G32B32_FLOAT:
		internalFormat = GL_RGB32F;
		targetFormat = GL_RGB;
		targetType   = GL_FLOAT;
		return true;

	case PIXEL_FORMAT_R32G32B32_UINT:
		internalFormat = GL_RGB32UI;
		targetFormat = GL_RGB;
		targetType   = GL_UNSIGNED_INT;
		return true;

	case PIXEL_FORMAT_R32G32B32_SINT:
		internalFormat = GL_RGB32I;
		targetFormat = GL_RGB;
		targetType   = GL_INT;
		return true;

	case PIXEL_FORMAT_R16G16B16A16_TYPELESS:
	case PIXEL_FORMAT_R16G16B16A16_FLOAT:
		internalFormat = GL_RGBA16F;
		targetFormat = GL_RGBA;
		targetType   = GL_HALF_FLOAT;
		return true;

	case PIXEL_FORMAT_R16G16B16A16_UNORM:
		internalFormat = GL_RGBA16;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16G16B16A16_UINT:
		internalFormat = GL_RGBA16UI;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16G16B16A16_SNORM:
		internalFormat = GL_RGBA16;
		targetFormat = GL_RGBA;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R16G16B16A16_SINT:
		internalFormat = GL_RGBA16I;
		targetFormat = GL_RGBA;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R32G32_TYPELESS:
	case PIXEL_FORMAT_R32G32_FLOAT:
		internalFormat = GL_RG32F;	// GLES has an extension for floating point textures http://www.khronos.org/registry/gles/extensions/OES/OES_texture_float.txt
		targetFormat = GL_RG;
		targetType   = GL_FLOAT;
		return true;

	case PIXEL_FORMAT_R32G32_UINT:
		internalFormat = GL_RG32UI;
		targetFormat = GL_RG;
		targetType   = GL_UNSIGNED_INT;
		return true;

	case PIXEL_FORMAT_R32G32_SINT:
		internalFormat = GL_RG32I;
		targetFormat = GL_RG;
		targetType   = GL_INT;
		return true;

	case PIXEL_FORMAT_R10G10B10A2_TYPELESS:
	case PIXEL_FORMAT_R10G10B10A2_UNORM:
		internalFormat = GL_RGB10_A2;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_INT_10_10_10_2;
		return true;

	case PIXEL_FORMAT_R10G10B10A2_UINT:
		internalFormat = GL_RGB10_A2UI;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_INT_10_10_10_2;
		return true;

	case PIXEL_FORMAT_R11G11B10_FLOAT:
		internalFormat = GL_R11F_G11F_B10F;
		targetFormat = GL_RGB;
		targetType   = GL_R11F_G11F_B10F; // <- is this correct?
		return true;

	case PIXEL_FORMAT_R8G8B8A8_TYPELESS:
	case PIXEL_FORMAT_R8G8B8A8_UNORM:
	case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
		internalFormat = GL_SRGB8_ALPHA8_EXT;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;
			
	case PIXEL_FORMAT_R8G8B8A8_UINT:
		internalFormat = GL_RGBA8UI;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;
			
	case PIXEL_FORMAT_R8G8B8A8_SNORM:
		internalFormat = GL_RGBA8_SNORM;
		targetFormat = GL_RGBA;
		targetType   = GL_BYTE;
		return true;
			
	case PIXEL_FORMAT_R8G8B8A8_SINT:
		internalFormat = GL_RGBA8I;
		targetFormat = GL_RGBA;
		targetType   = GL_BYTE;
		return true;
			
	case PIXEL_FORMAT_R16G16_TYPELESS:
	case PIXEL_FORMAT_R16G16_FLOAT:
		internalFormat = GL_RG16F;
		targetFormat = GL_RG;
		targetType   = GL_HALF_FLOAT;
		return true;

	case PIXEL_FORMAT_R16G16_UNORM:
		internalFormat = GL_RG16;
		targetFormat = GL_RG;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16G16_UINT:
		internalFormat = GL_RG16UI;
		targetFormat = GL_RG;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16G16_SNORM:
		internalFormat = GL_RG16_SNORM;
		targetFormat = GL_RG;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R16G16_SINT:
		internalFormat = GL_RG16I;
		targetFormat = GL_RG;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R32_TYPELESS:
	case PIXEL_FORMAT_R32_FLOAT:
		internalFormat = GL_R32F;	// GLES has an extension for floating point textures http://www.khronos.org/registry/gles/extensions/OES/OES_texture_float.txt
		targetFormat = GL_LUMINANCE;
		targetType   = GL_FLOAT;
		return true;

	case PIXEL_FORMAT_R32_UINT:
		internalFormat = GL_R32UI;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_UNSIGNED_INT;
		return true;

	case PIXEL_FORMAT_R32_SINT:
		internalFormat = GL_R32I;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_INT;
		return true;

	case PIXEL_FORMAT_R8G8_TYPELESS:
	case PIXEL_FORMAT_R8G8_UNORM:
		internalFormat = GL_SLUMINANCE8_ALPHA8_EXT;
		targetFormat = GL_LUMINANCE_ALPHA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8G8_UINT:
		internalFormat = GL_RG8UI;
		targetFormat = GL_LUMINANCE_ALPHA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8G8_SNORM:
		internalFormat = GL_RG8_SNORM;
		targetFormat = GL_LUMINANCE_ALPHA;
		targetType   = GL_BYTE;
		return true;

	case PIXEL_FORMAT_R8G8_SINT:
		internalFormat = GL_RG8I;
		targetFormat = GL_LUMINANCE_ALPHA;
		targetType   = GL_BYTE;
		return true;

	case PIXEL_FORMAT_R16_TYPELESS:
	case PIXEL_FORMAT_R16_FLOAT:
		internalFormat = GL_LUMINANCE16F_ARB;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_HALF_FLOAT;
		return true;

	case PIXEL_FORMAT_R16_UNORM:
		internalFormat = GL_LUMINANCE16;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16_UINT:
		internalFormat = GL_R16UI;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16_SNORM:
		internalFormat = GL_R16_SNORM;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R16_SINT:
		internalFormat = GL_R16I;
		targetFormat = GL_LUMINANCE;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R8_TYPELESS:
	case PIXEL_FORMAT_R8_UNORM:
		internalFormat = GL_SLUMINANCE_EXT;
		targetFormat = GL_LUMINANCE;
		targetType = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8_UINT:
		internalFormat = GL_R8UI;
		targetFormat = GL_LUMINANCE;
		targetType = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8_SNORM:
		internalFormat = GL_R8_SNORM;
		targetFormat = GL_LUMINANCE;
		targetType = GL_BYTE;
		return true;

	case PIXEL_FORMAT_R8_SINT:
		internalFormat = GL_R8I;
		targetFormat = GL_LUMINANCE;
		targetType = GL_BYTE;
		return true;

	case PIXEL_FORMAT_A8_UNORM:
		internalFormat = GL_SRGB8_ALPHA8_EXT;
		targetFormat = GL_ALPHA;
		targetType = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_BC1_TYPELESS:
	case PIXEL_FORMAT_BC1_UNORM:
	case PIXEL_FORMAT_BC1_UNORM_SRGB:
		internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		targetFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		targetType   = 0;
		return true;
			
	case PIXEL_FORMAT_BC2_TYPELESS:
	case PIXEL_FORMAT_BC2_UNORM:
	case PIXEL_FORMAT_BC2_UNORM_SRGB:
		internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		targetFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		targetType   = 0;
		return true;
			
	case PIXEL_FORMAT_BC3_TYPELESS:
	case PIXEL_FORMAT_BC3_UNORM:
	case PIXEL_FORMAT_BC3_UNORM_SRGB:
		internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		targetFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		targetType   = 0;
		return true;
			
	case PIXEL_FORMAT_B5G6R5_UNORM:
		internalFormat = GL_RGB;
		targetFormat = GL_RGB;
		targetType   = GL_UNSIGNED_SHORT_5_6_5;
		return true;

	case PIXEL_FORMAT_B5G5R5A1_UNORM:
		internalFormat = GL_RGBA;
		targetFormat = GL_RGBA;
		targetType   = GL_UNSIGNED_SHORT_5_5_5_1;
		return true;

	case PIXEL_FORMAT_B8G8R8A8_UNORM:
	case PIXEL_FORMAT_B8G8R8A8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
		internalFormat = GL_SRGB8_ALPHA8_EXT;
		targetFormat = GL_BGRA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_B8G8R8X8_UNORM:
	case PIXEL_FORMAT_B8G8R8X8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
		internalFormat = GL_SRGB8_EXT;
		targetFormat = GL_BGRA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;
#endif
	default:
		return false;
	}
	
}

void Tr2RenderContextAL::ReleaseDeviceResources()
{
	delete m_blitter;
	m_blitter = nullptr;
}

bool Tr2RenderContextAL::ApplyVertexDeclaration( ShadowStateRestoreInfo& info, const void* data, size_t stride )
{
	static const GLenum elemType[ 16 ] =
	{
		GL_BYTE,
		GL_SHORT,
		GL_INT,
		GL_HALF_FLOAT,
		GL_FLOAT,
		0, 0, 0,

		// unsigned bit
		GL_UNSIGNED_BYTE,
		GL_UNSIGNED_SHORT,
		GL_UNSIGNED_INT,
		GL_HALF_FLOAT,
		GL_FLOAT,
		0, 0, 0,		
	};

	if( m_boundLayout == nullptr || !m_program.IsValid() )
	{
		return false;
	}

	bool patchedPS = ( m_alphaTestParameters.m_alphaTestEnabled &&
		m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS ) ||
		m_fragmentOpSettings.m_clipPlaneEnable;
	auto programData = patchedPS && m_program.m_program->m_patchedProgram.program ? &m_program.m_program->m_patchedProgram : &m_program.m_program->m_program;

	unsigned currentArray = 8;
	auto& definition = *m_boundLayout;
	info.m_instanceAttributesCount = 0;

	if( data )
	{
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	}

	for( size_t i = 0; i != definition.m_items.size(); ++i )
    {
		const Tr2VertexDefinition::Item& src = definition.m_items[i];
		if( src.m_stream >= 8 )
		{
			return false;
		}
		if( data )
		{
			if( src.m_stream > 0 )
			{
				return false;
			}
		}
		auto it = programData->attributes.find( std::make_pair( src.m_usage, src.m_usageIndex ) );
		if( it != programData->attributes.end() )
		{
			if( !data )
			{
				if( !m_boundStreams[src.m_stream].buffer )
				{
					return false;
				}
				if( src.m_stream != currentArray )
				{
					glBindBuffer( GL_ARRAY_BUFFER, m_boundStreams[src.m_stream].buffer );
					currentArray = src.m_stream;
				}
			}
			glEnableVertexAttribArray( it->second ); 

			uintptr_t offset = src.m_offset;
			glVertexAttribPointer( it->second, 
									definition.GetDataTypeSizeInMembers( src.m_dataType ),
									elemType[ src.m_dataType & ( Tr2VertexDefinition::DT_TYPE_MASK | Tr2VertexDefinition::DT_UNSIGNED_BIT ) ], 
									src.m_dataType & Tr2VertexDefinition::DT_NORMALIZED_BIT ? GL_TRUE : GL_FALSE,
									GLsizei( data ? stride : m_boundStreams[src.m_stream].stride ), 
									static_cast<const uint8_t*>( data ) + offset );
			if( src.m_instanceStepRate )
			{
				glVertexAttribDivisorARB( it->second, 1 );
				info.m_instanceAttributes[info.m_instanceAttributesCount++] = it->second;
			}
		}
    }

	return true;
}

bool Tr2RenderContextAL::ApplyShadowRenderStates( ShadowStateRestoreInfo& )
{
	m_fragmentOpSettings.UpdateContents( m_alphaTestParameters );
	CR_RETURN_VAL( SetProgram(), false );

	bool patchedPS = ( m_alphaTestParameters.m_alphaTestEnabled &&
		m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS ) ||
		m_fragmentOpSettings.m_clipPlaneEnable;
	auto programData = patchedPS && m_program.m_program->m_patchedProgram.program ? &m_program.m_program->m_patchedProgram : &m_program.m_program->m_program;

	for( unsigned i = 0; i != 16; ++i )
	{
		if( m_boundBuffers[i].IsValid() && programData->constantBuffers[i] >= 0 )
		{
			glUniform4fv( 
				programData->constantBuffers[i],
				m_boundBuffers[i].GetSize() / 4 / sizeof( float ), 
				(float*)m_boundBuffers[i].m_buffer->m_shadowCopy.get() );
		}
	}
	if( programData->intConstant >= 0 )
	{
		glUniform4i( programData->intConstant, m_numberOfLights, 0, 0, 0 );
	}

	if( programData->shadowStateInt >= 0 )
	{
		glUniform4f( 
			programData->shadowStateInt,
			float( m_fragmentOpSettings.m_invertedAlphaTest ),
			float( m_fragmentOpSettings.m_alphaTestRef ),
			float( m_fragmentOpSettings.m_alphaTestFunc ),
			float( m_fragmentOpSettings.m_clipPlaneEnable )
			);
		float clipPlane[Tr2FragmentOpSettings::MAX_CLIP_PLANES][4];
		for( int i = 0; i < Tr2FragmentOpSettings::MAX_CLIP_PLANES; ++i )
		{
			if( m_fragmentOpSettings.m_clipPlaneEnable & ( 1 << i ) )
			{
				std::copy( m_fragmentOpSettings.m_clipPlane[i], m_fragmentOpSettings.m_clipPlane[i] + 4, clipPlane[i] );
			}
			else
			{
				clipPlane[i][0] = 0.0f;
				clipPlane[i][1] = 0.0f;
				clipPlane[i][2] = 0.0f;
				clipPlane[i][3] = 0.0f;
			}
		}
		glUniform4fv( 
			programData->shadowStateFloat,
			m_fragmentOpSettings.MAX_CLIP_PLANES,
			(GLfloat*)clipPlane );
	}
	if( programData->shadowStateOffsets >= 0 )
	{
		glUniform3f( 
			programData->shadowStateOffsets,
			1.0f / float( m_currentViewport.m_width ),
			-1.0f / float( m_currentViewport.m_height ),
			-1.f );
	}

	if( m_blendState.dirty )
	{
		if( m_blendState.separateAlphaBlend )
		{
			CR_GL_RETURN_VAL( glBlendEquationSeparate( m_blendState.blendMode, m_blendState.blendModeAlpha )
							, false );

			CR_GL_RETURN_VAL( glBlendFuncSeparate(	m_blendState.srcBlend, 
													m_blendState.destBlend, 
													m_blendState.srcBlendAlpha, 
													m_blendState.destBlendAlpha )
							, false );
		}
		else
		{
			CR_GL_RETURN_VAL( glBlendEquation( m_blendState.blendMode )
							, false );

			CR_GL_RETURN_VAL( glBlendFunc( m_blendState.srcBlend, m_blendState.destBlend )
							, false );
		}
		m_blendState.dirty = false;
	}
	if( m_stencilOpState.dirty )
	{
		if( m_stencilOpState.twoSided )
		{
			CR_GL_RETURN_VAL( glStencilOpSeparate(	GL_FRONT, 
													m_stencilOpState.stencilFail, 
													m_stencilOpState.stencilZFail, 
													m_stencilOpState.stencilPass )
							, false );

			CR_GL_RETURN_VAL( glStencilOpSeparate(	GL_BACK, 
													m_stencilOpState.ccwStencilFail, 
													m_stencilOpState.ccwStencilZFail, 
													m_stencilOpState.ccwStencilPass )
							, false );
		}
		else
		{
			CR_GL_RETURN_VAL( glStencilOp(			m_stencilOpState.stencilFail, 
													m_stencilOpState.stencilZFail, 
													m_stencilOpState.stencilPass )
							, false );
		}
		m_stencilOpState.dirty = false;
	}
	if( m_stencilTestState.dirty )
	{
		if( m_stencilTestState.twoSided )
		{
			CR_GL_RETURN_VAL( glStencilFuncSeparate(	GL_FRONT, 
														m_stencilTestState.func, 
														m_stencilTestState.ref, 
														m_stencilTestState.mask )
							, false );

			CR_GL_RETURN_VAL( glStencilFuncSeparate(	GL_BACK, 
														m_stencilTestState.ccwFunc, 
														m_stencilTestState.ccwRef, 
														m_stencilTestState.ccwMask )
							, false );
		}
		else
		{
			CR_GL_RETURN_VAL( glStencilFunc(	m_stencilTestState.func, 
												m_stencilTestState.ref, 
												m_stencilTestState.mask )
							, false );
		}
		m_stencilTestState.dirty = false;
	}
	if( m_depthOffsetState.dirty )
	{
		CR_GL_RETURN_VAL( glPolygonOffset(	m_depthOffsetState.slopeScaleOffset, 
											m_depthOffsetState.bias )
						, false );

		m_depthOffsetState.dirty = false;
	}

	return true;
}

void Tr2RenderContextAL::RestoreState( const ShadowStateRestoreInfo& info )
{
	for( unsigned i = 0; i < info.m_instanceAttributesCount; ++i )
	{
		glVertexAttribDivisorARB( info.m_instanceAttributes[i], 0 );
	}
}

ALResult Tr2RenderContextAL::SetRtDsToDevice( uint32_t changedSlot )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	uint32_t colorBuffer[MAX_RENDER_TARGET] = { 0 };
	uint32_t colorBufferType[MAX_RENDER_TARGET] = { GL_TEXTURE_2D };
	bool allZeroColor = true;
	
	for( uint32_t i = 0; i != m_renderTargetHighWaterMark; ++i )
	{
		if( m_boundRenderTarget[i].IsValid() )
		{
			if( m_boundRenderTarget[i].m_texture->m_msaaBuffer )
			{
				colorBuffer[i] = m_boundRenderTarget[i].m_texture->m_msaaBuffer;
				colorBufferType[i] = GL_RENDERBUFFER;
			}
			else
			{
				colorBuffer[i] = m_boundRenderTarget[i].m_texture->m_texture;
				colorBufferType[i] = GL_TEXTURE_2D;
			}
		}
		else
		{
			colorBuffer[i] = 0;
			colorBufferType[i] = GL_TEXTURE_2D;
		}

		allZeroColor &= colorBuffer[i] == 0;
	}

	if( allZeroColor )
	{
		return E_FAIL;
	}

	if( m_boundDepthStencil.IsValid() && m_renderStates[RS_ZENABLE] )
	{
		glEnable( GL_DEPTH_TEST );
	}
	else
	{
		glDisable( GL_DEPTH_TEST );
	}

	const Tr2TextureAL& bb = m_boundRenderTarget[0].IsValid() ? m_boundRenderTarget[0] : m_defaultBackBuffer;

	// Msaa zero is the same as one, so does need to show up as 'compatible'
	const uint32_t dsMsaaType = m_boundDepthStencil.IsValid() ? std::max( m_boundDepthStencil.GetMsaaDesc().samples, 1u ) : 1;
	const uint32_t bbMsaaType = std::max( bb.GetMsaaDesc().samples, 1u );

	// dont't even bother setting it when the dimensions don't match, it's not gonna work.
	// This happens when we set/push/pop an RT and DS in two separate calls -- there's a point between
	// those two where it's in a bad state.  Silently "works" in DX9, complains in DX11. Fix the spam:
	if( !m_boundDepthStencil.IsValid()	||
		!( m_boundDepthStencil.GetWidth()		== bb.GetWidth()		&&
		  m_boundDepthStencil.GetHeight()		== bb.GetHeight()		&&
		  m_boundDepthStencil.GetMsaaDesc().samples == bb.GetMsaaDesc().quality	&&
		  dsMsaaType							== bbMsaaType
		) )
	{		
		GL_FAIL( glBindFramebuffer( GL_FRAMEBUFFER, m_defaultFrameBufferObject ) );
		for( uint32_t i = 0; i != m_renderTargetHighWaterMark; ++i )
		{
			if( colorBufferType[i] == GL_RENDERBUFFER )
			{
				GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, colorBufferType[i], colorBuffer[i] ) );
			}
			else
			{
				GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, colorBufferType[i], colorBuffer[i], 0 ) );
			}
		}
		if( m_boundDepthStencil.IsValid() )
		{
			if( m_boundDepthStencil.m_texture->m_texture )
			{
				GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_boundDepthStencil.m_texture->m_texture, 0 ) );
				if( m_boundDepthStencil.m_texture->m_stencilBuffer )
				{
					GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_boundDepthStencil.m_texture->m_stencilBuffer ) );
				}
				else
				{
					GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_boundDepthStencil.m_texture->m_texture, 0 ) );
				}
			}
			else
			{
				GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_boundDepthStencil.m_texture->m_msaaBuffer ) );
				GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_boundDepthStencil.m_texture->m_stencilBuffer ) );
			}
		}
		else
		{
			GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , GL_RENDERBUFFER, 0 ) );
			GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0 ) );
		}
#if defined(TRINITYDEV) || defined(_DEBUG)
		const uint32_t status = glCheckFramebufferStatus( GL_FRAMEBUFFER );

		switch( status )
		{
		case GL_FRAMEBUFFER_COMPLETE: 
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: 
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			CCP_AL_LOGWARN(" SetRtDsToDevice: combination of internal formats not supported" );
			return E_FAIL;
		default:
			CCP_AL_LOGWARN(" SetRtDsToDevice: unknown status" );
			break;
		}		
#endif
	}

	// emulate DX9 -- setting RT sets the VP, trinity relies on this
	// Note we do this always, even if we didn't change the device at this point because we don't have the matching depthStencil yet.
	// The assumption is that we'll get it, eventually.
	// In other words, avoid a dependency on the order in which people set RT/DS. Without this, RT+DS would be no viewport change,
	// versus DS+RT = would update the viewport.
	//
	// Similar with ScissorRect, see
	//	http://msdn.microsoft.com/en-us/library/windows/desktop/bb147354%28v=vs.85%29.aspx
	// " IDirect3DDevice9::SetRenderTarget resets the scissor rectangle to the full render target, analogous to the viewport reset. "
	//
	if( changedSlot == 0 )
	{
		SetViewport( Tr2Viewport ( bb.GetWidth(), bb.GetHeight() ) );
		glScissor( 0, 0, bb.GetWidth(), bb.GetHeight() );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::CopySubresourceRegion( 
	TrinityALImpl::Tr2TextureAL& destination,
	const Tr2TextureSubresource& destSubresource,
	TrinityALImpl::Tr2TextureAL& source,
	const Tr2TextureSubresource& sourceSubresource )
{
	if( !destination.IsValid() || !source.IsValid() || !IsValid() )
	{
		return E_FAIL;
	}


	Tr2TextureSubresource src = sourceSubresource;
	Tr2TextureSubresource dst = destSubresource;	
	
	if( !Crop( src, source.GetDesc(), dst, destination.GetDesc() ) )
	{
		return E_INVALIDARG;
	}

	uint32_t box[6] = { src.m_left, src.m_top, src.m_front, src.m_right, src.m_bottom, src.m_back };

	const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

	GL_FAIL( glBindFramebuffer( GL_FRAMEBUFFER, m_defaultFrameBufferObject ) );
	GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , GL_RENDERBUFFER, 0 ) );
	GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0 ) );
	ON_BLOCK_EXIT( 
		[&]{ 
			SetRtDsToDevice( 0 ); 
		} );
	for( uint32_t mip = 0; mip != mipCount; ++mip )
	{
		for( uint32_t face = 0; face != src.GetFaceCount(); ++face )
		{
			GLenum srcTexType = source.GetDesc().GetType() == TEX_TYPE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_TEXTURE_2D;
			GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, srcTexType, source.m_texture, src.m_startMipLevel + mip ) );

			//SetViewport( Tr2Viewport ( source.GetMipWidth( src.m_startMipLevel + mip ), source.GetMipHeight( src.m_startMipLevel + mip ) ) );
			//SetScissorRect( 0, 0, source.GetMipWidth( src.m_startMipLevel + mip ), source.GetMipHeight( src.m_startMipLevel + mip ) );

			GLenum texType = destination.GetDesc().GetType() == TEX_TYPE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_TEXTURE_2D;
			uint32_t height = destination.GetDesc().GetMipHeight( dst.m_startMipLevel + mip );
			GL_FAIL( glBindTexture( texType, destination.m_texture ) );
			GL_FAIL( glCopyTexSubImage2D( texType, dst.m_startMipLevel + mip, dst.m_left, dst.m_top, box[0], height - box[4], box[3] - box[0], box[4] - box[1] ) );
		}

		if( mip + 1 != src.GetMipCount() )
		{
			AdvanceMip( src, source.GetDesc(), mip );
			AdvanceMip( dst, destination.GetDesc(),  mip );
		}

		for( uint32_t k = 0; k < 6; ++k )
		{
			box[k] = box[k] >> 1;
		}
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::InternalResolveRT( TrinityALImpl::Tr2TextureAL& destination, const TrinityALImpl::Tr2TextureAL& source )
{
#if defined(TRINITY_AL_MOBILE)
    return E_FAIL;
#else
	if( !IsValid() || !destination.IsValid() || !source.IsValid() || !destination.m_texture )
	{
		return E_INVALIDARG;
	}
	CR_GL( glBindFramebuffer( GL_FRAMEBUFFER_EXT, 0 ) );
	CR_GL( glBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, m_offscreenFrameBuffer0 ) );
	if( source.m_msaaBuffer )
	{
		CR_GL( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, source.m_msaaBuffer ) );
	}
	else
	{
		CR_GL( glFramebufferTexture2D( GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source.m_texture, 0 ) );
	}
#if defined(TRINITYDEV) || defined(_DEBUG)
	if( glCheckFramebufferStatus( GL_READ_FRAMEBUFFER_EXT ) != GL_FRAMEBUFFER_COMPLETE )
	{
		CCP_AL_LOGERR( "Tr2RenderContextAL::InternalResolveRT: invalid read buffer" );
		return E_FAIL;
	}
#endif
	CR_GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, m_offscreenFrameBuffer1 ) );
	CR_GL( glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destination.m_texture, 0 ) );
#if defined(TRINITYDEV) || defined(_DEBUG)
	if( glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER_EXT ) != GL_FRAMEBUFFER_COMPLETE )
	{
		CCP_AL_LOGERR( "Tr2RenderContextAL::InternalResolveRT: invalid draw buffer" );
		return E_FAIL;
	}
#endif
	CR_GL( glBlitFramebuffer( 0, 0, source.GetDesc().GetWidth(), source.GetDesc().GetHeight(), 0, 0, destination.GetDesc().GetWidth(), destination.GetDesc().GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
	CR_GL( glBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, 0 ) );
	CR_GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, 0 ) );
	CR_GL( glBindFramebuffer( GL_FRAMEBUFFER, m_defaultFrameBufferObject ) ); 
	return S_OK;
#endif
}

// --------------------------------------------------------------------------------------
void Tr2RenderContextAL::AddGpuMarker( const char* )
{
}

void Tr2RenderContextAL::PushGpuMarker( const char* )
{
}

void Tr2RenderContextAL::PopGpuMarker()
{
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus&, std::string& ) const
{
	return E_FAIL;
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuPageFaultResource(
	Tr2RenderContextEnum::PixelFormat&,
	uint64_t&,
	uint32_t&,
	uint32_t&,
	uint32_t&,
	uint32_t& ) const
{
	return E_FAIL;
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

#endif
