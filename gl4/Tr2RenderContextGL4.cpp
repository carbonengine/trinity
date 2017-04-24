#include "StdAfx.h"
#include "Tr2RenderContextGL4.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "ITr2RenderContextEvents.h"
#include "ALLog.h"
#if !defined(_WIN32)
#include "GLFW/glfw3.h"
#else
#include "wglext.h"
#endif

using namespace Tr2RenderContextEnum;
#pragma warning( disable: 4189 )	// Scopeguard

CCP_STATS_DECLARE( primitiveCount		, "Trinity/AL/primitiveCount"		, true, CST_COUNTER_HIGH, "Primitive count in DrawPrimitive calls." );
CCP_STATS_DECLARE( vertexCount			, "Trinity/AL/vertexCount"			, true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );
CCP_STATS_DECLARE( sceneDrawcallCount	, "Trinity/AL/sceneDrawcallCount"	, true, CST_COUNTER_LOW,  "Number of DrawPrimitive calls." );

#define	INVALID_WIN32	0xFFFFFFFF
#define	INVALID_HRC  	(HGLRC)0xFFFFFFFF

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

}

static GLenum ConvertTextureType( TextureType type )
{
	switch( type )
	{
	case TEX_TYPE_CUBE:
		return GL_TEXTURE_CUBE_MAP;
	case TEX_TYPE_3D:
		return GL_TEXTURE_3D;
	default:
		return GL_TEXTURE_2D;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Internal helper class for blitting a texture into the frame buffer
// --------------------------------------------------------------------------------------
struct Tr2RenderContextAL::Blitter
{
	Blitter()
		:m_pipeline( 0 ),
		m_vertexShader( 0 ),
		m_pixelShader( 0 )
	{
	}

	~Blitter()
	{
		if( m_pipeline )
		{
			glDeleteProgramPipelines( 1, &m_pipeline );
		}
		if( m_vertexShader )
		{
			glDeleteProgram( m_vertexShader );
		}
		if( m_pixelShader )
		{
			glDeleteProgram( m_pixelShader );
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
		const char* vsSource =
            "#version 410 core\n"
			"out gl_PerVertex { vec4 gl_Position; };\n"
			"layout(location=0) in vec4 aPos;\n"
			"layout(location=0) out vec2 vTex;\n"
			"void main()\n"
			"{\n"
			"vTex.xy=aPos.zw;\n"
			"gl_Position=vec4(aPos.xy,0.0,1.0);\n"
			"}";
		m_vertexShader = glCreateShaderProgramv( GL_VERTEX_SHADER, 1, &vsSource );
		if( !m_vertexShader )
		{
			return E_FAIL;
		}
		GLint status = 0;
		GL_FAIL( glGetProgramiv( m_vertexShader, GL_LINK_STATUS, &status ) );
		if( !status )
		{
			GLint length;
			glGetProgramiv( m_vertexShader, GL_INFO_LOG_LENGTH, &length );
			char* buffer = new char[length];
			glGetProgramInfoLog( m_vertexShader, length, nullptr, buffer );
			CCP_AL_LOGERR( "Tr2ShaderAL: error compiling shader: %s", buffer );
			delete[] buffer;
			return E_FAIL;
		}

        const char* fsSource =
            "#version 410 core\n"
            "layout(location=0) in vec2 vTex;\n"
			"uniform sampler2D tex;\n"
            "layout(location=0) out vec4 FragColor;\n"
			"void main()\n"
			"{\n"
			"FragColor=texture(tex, vTex);\n"
			"}";
		m_pixelShader = glCreateShaderProgramv( GL_FRAGMENT_SHADER, 1, &fsSource );
		GL_FAIL( glGetProgramiv( m_pixelShader, GL_LINK_STATUS, &status ) );
		if( !status )
		{
			GLint length;
			glGetProgramiv( m_pixelShader, GL_INFO_LOG_LENGTH, &length );
			char* buffer = new char[length];
			glGetProgramInfoLog( m_pixelShader, length, nullptr, buffer );
			CCP_AL_LOGERR( "Tr2ShaderAL: error compiling shader: %s", buffer );
			delete[] buffer;
			return E_FAIL;
		}
		glGenProgramPipelines( 1, &m_pipeline );
		glUseProgramStages( m_pipeline, GL_VERTEX_SHADER_BIT, m_vertexShader );
		glUseProgramStages( m_pipeline, GL_FRAGMENT_SHADER_BIT, m_pixelShader );

		GLint location = glGetUniformLocation( m_pixelShader, "tex" );
		GL_FAIL( glProgramUniform1i( m_pixelShader, location, 0 ) );

		float border[4] = { 0 };
		Tr2SamplerDescription desc( 
			TF_POINT, 
			TF_POINT, 
			TF_POINT, 
			false, 
			TA_CLAMP, 
			TA_CLAMP, 
			TA_CLAMP, 
			0.0f, 
			1, 
			CMP_ALWAYS, 
			border, 
			0, 
			std::numeric_limits<float>::max() );
		CR_RETURN_HR( m_sampler.Create( renderContext, desc ) );
		GL_IGNORE_ERROR( glSamplerParameteri( m_sampler.m_sampler, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT ) );

		float vb[] = {
			-1.0f, -1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 0.0f,
		};
		CR_RETURN_HR( m_buffer.Create( sizeof( float ) * 16, USAGE_IMMUTABLE, vb, renderContext ) );
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
		AL_UPDATE_RESOURCE_FRAME_USAGE( m_sampler );
		AL_UPDATE_RESOURCE_FRAME_USAGE( m_buffer );

		glBindProgramPipeline( m_pipeline );

		GL_FAIL( glActiveTexture( GL_TEXTURE0 ) );
		GL_FAIL( glBindTexture( GL_TEXTURE_2D, *source.m_texture ) );
		GL_FAIL( glBindSampler( 0, m_sampler.m_sampler ) );

		GL_FAIL( glBindBuffer( GL_ARRAY_BUFFER, m_buffer.m_buffer ) );
		GL_FAIL( glEnableVertexAttribArray( 0 ) );
		GL_FAIL( glVertexAttribPointer( 0,
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
	GLuint m_vertexShader;
	GLuint m_pixelShader;
	GLuint m_pipeline;
	Tr2VertexBufferAL m_buffer;
};

Tr2RenderContextAL::Tr2RenderContextAL()
	: m_topology( TOP_TRIANGLES )
	, m_renderTargetHighWaterMark( 1 )
	, m_stackDS( "Tr2RenderContextAL::m_stackDS" )
	, m_boundIndexBufferIs16Bit( false )
	, m_boundLayout( nullptr )
	, m_boundDepthStencil( nullptr )
	, m_vertexShader( nullptr )
	, m_pixelShader( nullptr )
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
	, m_currentActiveTexture( 0 )
	, m_blitter( nullptr )
	, m_events( nullptr )
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() == nullptr );
	::GetPrimaryRenderContextPointer() = this;

	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTarget[i] = nullptr;
		m_stackRT[i].SetName( "Tr2RenderContextAL::m_stackRT" );
	}

	for( unsigned j = 0; j < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++j )
	{
		for( unsigned i = 0; i < 16; ++i )
		{
			m_boundBuffers[j][i] = nullptr;
		}
	}
	for( unsigned i = 0; i < 16; ++i )
	{
		m_boundUavs[i] = nullptr;
		m_boundTextures[i] = nullptr;
		m_boundClBuffers[i] = nullptr;
		m_boundClSamplers[i] = nullptr;
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
	m_pipeline = 0;

	m_clQueue = nullptr;
	m_clContext = nullptr;
	m_clDevice = nullptr;
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

	m_defaultBackBuffer.Destroy();
	m_defaultDepthStencil.Destroy();

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
	glDeleteProgramPipelines( 1, &m_pipeline );
	m_drawUP.Destroy();

	ReleaseDeviceResources();

	ReleaseOpenGLContext();

	m_topology = TOP_TRIANGLES;

	m_boundLayout = nullptr;
	m_vertexShader = nullptr;
	m_pixelShader = nullptr;

	for( unsigned i = 0; i < 8; ++i )
	{
		m_boundStreams[i].buffer = 0;
	}
	for( unsigned i = 0; i < 16; ++i )
	{
		if( m_boundUavs[i] )
		{
			clReleaseMemObject( m_boundUavs[i] );
		}
		m_boundUavs[i] = nullptr;
		if( m_boundTextures[i] )
		{
			clReleaseMemObject( m_boundTextures[i] );
		}
		m_boundTextures[i] = nullptr;
		if( m_boundClBuffers[i] )
		{
			clReleaseMemObject( m_boundClBuffers[i] );
		}
		m_boundClBuffers[i] = nullptr;
		if( m_boundClSamplers[i] )
		{
			clReleaseSampler( m_boundClSamplers[i] );
		}
		m_boundClSamplers[i] = nullptr;
	}
	if( m_clQueue )
	{
		clReleaseCommandQueue( m_clQueue );
		m_clQueue = nullptr;
	}
	if( m_clContext )
	{
		clReleaseContext( m_clContext );
		m_clContext = nullptr;
	}
	m_clDevice = nullptr;

	m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_alphaTestParameters.m_alphaTestRef = 0;
	m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;
	m_fragmentOpSettings.m_clipPlaneEnable = 0;

	m_currentActiveTexture = 0;

	m_hWnd = 0;
}

ALResult Tr2RenderContextAL::SetStreamSource( 
	uint32_t stream, 
	const Tr2VertexBufferAL & buffer, 
	uint32_t offset, 
	uint32_t stride )
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
		if( &buffer == &nullVB )
		{
			return S_OK;
		}
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
	m_boundStreams[stream].buffer = buffer.m_buffer;
	m_boundStreams[stream].stride = stride;
	m_boundStreams[stream].offset = offset;
	return S_OK;
}

ALResult Tr2RenderContextAL::Clear(	
	uint32_t clearFlags, 
	uint32_t color, 
	float depth, 
	uint32_t stencil,
	uint32_t slot )
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
		glClearDepth( depth );
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

// Version of SetIndices that accepts a nullpointer, in which case the currently bound index buffer is un-set.
ALResult Tr2RenderContextAL::SetIndices( const Tr2IndexBufferAL& buffer )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}
	if( !buffer.IsValid() )
	{
		GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
		return S_OK;
	}
	
	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
	m_boundIndexBufferIs16Bit = buffer.Is16Bit();
	GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer.m_buffer ) );
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
		CCP_ASSERT( false && "Unsupported topology" );
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
	CR_GL( glDrawElementsInstanced(
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
	return m_drawUP.DrawPrimitiveUP(	primitiveCount,
										vertexStreamZeroData,
										vertexStreamZeroStride,
										*this );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	return m_drawUP.DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride,
											*this );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	return m_drawUP.DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride,
											*this );
}

ALResult Tr2RenderContextAL::RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ ) throw()
{
	if( !m_computeShader )
	{
		return E_INVALIDCALL;
	}

	auto& def = m_computeShader->GetInputDefinition();
	std::vector<cl_mem> inputs;
	inputs.reserve( def.elements.size() );
	for( cl_uint i = 0; i < def.elements.size(); ++i )
	{
		cl_mem input = nullptr;
		switch( def.elements[i].usageIndex )
		{
		case 0:
			input = m_boundUavs[def.elements[i].registerIndex];
			break;
		case 1:
			input = m_boundTextures[def.elements[i].registerIndex];
			break;
		case 2:
			input = m_boundClBuffers[def.elements[i].registerIndex];
			break;
		case 3:
			{
				auto sampler = m_boundClSamplers[def.elements[i].registerIndex];
				if( !sampler )
				{
					return E_INVALIDCALL;
				}
				clSetKernelArg( m_computeShader->m_clKernel, i, sizeof( sampler ), &sampler );
				continue;
			}
		default:
			return E_INVALIDCALL;
		}
		if( !input )
		{
			return E_INVALIDCALL;
		}
		clSetKernelArg( m_computeShader->m_clKernel, i, sizeof( input ), &input );
		if( def.elements[i].usageIndex != 2 )
		{
			inputs.push_back( input );
		}
	}

	glFinish();
	clEnqueueAcquireGLObjects( m_clQueue, cl_uint( inputs.size() ), &inputs[0], 0, nullptr, nullptr );

	size_t globalWorkSize[] = { groupDimX, groupDimY, groupDimZ };

	size_t workgroup[3];
	clGetKernelWorkGroupInfo( m_computeShader->m_clKernel, m_clDevice, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 3 * sizeof( size_t ), workgroup, nullptr );
	if( !workgroup[0] )
	{
		workgroup[0] = 1;
		workgroup[1] = 1;
		workgroup[2] = 1;
	}
	globalWorkSize[0] *= workgroup[0];
	globalWorkSize[1] *= workgroup[1];
	globalWorkSize[2] *= workgroup[2];
	auto res = clEnqueueNDRangeKernel( m_clQueue, m_computeShader->m_clKernel, 3, NULL, globalWorkSize, workgroup, 0, NULL, NULL);
	 
	clEnqueueReleaseGLObjects( m_clQueue, cl_uint( inputs.size() ), &inputs[0], 0, nullptr, nullptr );
	clFinish( m_clQueue );

	if( res == CL_SUCCESS )
	{
		return S_OK;
	}
	return E_FAIL;
}

ALResult Tr2RenderContextAL::SetConstants(	
	Tr2ConstantBufferAL& buffer, 
	ShaderType constantType, 
	uint32_t registerIndex, 
	uint32_t /*maxRegisterCount*/ )
{
	if( registerIndex >= 16 )
	{
		return E_INVALIDARG;
	}

	if( constantType == COMPUTE_SHADER )
	{
		cl_mem mem = nullptr;
		if( !buffer.IsValid() )
		{
			if( &buffer != &nullCB )
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
			mem = clCreateBuffer( m_clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer.GetSize(), buffer.m_shadowCopy.get(), nullptr );
			if( !mem )
			{
				return E_FAIL;
			}
		}
		if( m_boundClBuffers[registerIndex] )
		{
			clReleaseMemObject( m_boundClBuffers[registerIndex] );
		}
		m_boundClBuffers[registerIndex] = mem;
		return S_OK;
	}


	if( !buffer.IsValid() )
	{
		m_boundBuffers[constantType][registerIndex] = nullptr;
		if( &buffer == &nullCB )
		{
			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
	m_boundBuffers[constantType][registerIndex] = &buffer;

	return S_OK;
}

ALResult Tr2RenderContextAL::SetSamplerState( 
	const Tr2SamplerStateAL& samplerState, 
	ShaderType inputType, 
	uint32_t registerNumber )
{
	if( registerNumber >= 16 )
	{
		return E_INVALIDARG;
	}
	if( inputType == COMPUTE_SHADER )
	{
		cl_sampler mem = nullptr;
		if( samplerState.IsValid() )
		{
			AL_UPDATE_RESOURCE_FRAME_USAGE( samplerState );
			if( !samplerState.m_clObject )
			{
				samplerState.m_clObject = clCreateSampler( m_clContext, CL_TRUE, 
					samplerState.m_stateData.m_wrapT == GL_TEXTURE_WRAP_S ? CL_ADDRESS_REPEAT : CL_ADDRESS_CLAMP, 
					samplerState.m_stateData.m_magFilter == GL_NEAREST ? CL_FILTER_NEAREST : CL_FILTER_LINEAR, 
					nullptr );
				if( !samplerState.m_clObject )
				{
					return E_FAIL;
				}
			}
			mem = samplerState.m_clObject;
		}
		if( m_boundClSamplers[registerNumber] )
		{
			clReleaseSampler( m_boundClSamplers[registerNumber] );
		}
		m_boundClSamplers[registerNumber] = mem;
		if( mem )
		{
			clRetainSampler( mem );
		}
		return S_OK;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( samplerState );

	GL_FAIL( glBindSampler( registerNumber, samplerState.m_sampler ) );
	if( samplerState.m_sampler )
	{
		GL_FAIL( glSamplerParameteri( samplerState.m_sampler,
									GL_TEXTURE_SRGB_DECODE_EXT, 
									m_srgbDecode[inputType][registerNumber] ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT  ) );
	}
	m_boundSamplers[inputType][registerNumber] = samplerState.m_sampler;
	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool /*enable*/ ) {}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return false;
}
ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2DepthStencilAL& depthStencil )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( depthStencil );
	m_boundDepthStencil = depthStencil.IsValid() ? &depthStencil : nullptr;
	
	return SetRtDsToDevice( MAX_RENDER_TARGET );
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( renderTarget );
	m_boundRenderTarget[slot] = renderTarget.IsValid() ? &renderTarget : nullptr;
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

	void OnClError( const char *errinfo, const void *private_info, size_t cb, void *user_data )
	{
		CCP_AL_LOGERR( "OpenCL error: %s", errinfo );
	}

	template<typename Func, typename Obj, typename Enum>
	std::string GetClString( Func func, Obj obj, Enum arg )
	{
		size_t length;
		if( func( obj, arg, 0, nullptr, &length ) != CL_SUCCESS )
		{
			return "";
		}
		std::string result;
		result.resize( length );
		if( func( obj, arg, length, &result[0], nullptr ) != CL_SUCCESS )
		{
			return "";
		}
		return result;
	}
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
	if( pp.windowed )
	{
#ifdef _WIN32
		RECT rect;
		GetClientRect( pp.outputWindow, &rect );
		pp.mode.width = std::max( rect.right - rect.left, 8l );
		pp.mode.height = std::max( rect.bottom - rect.top, 8l );
#else
		int w, h;
		glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( pp.outputWindow ), &w, &h );
		pp.mode.width = uint32_t( w );
		pp.mode.height = uint32_t( h );
#endif
	}
	CR_RETURN_HR( SetPresentParameters( adapter, pp ) );

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray( vao );

	glGenProgramPipelines( 1, &m_pipeline );
	glBindProgramPipeline( m_pipeline );

	glEnable( GL_CULL_FACE );
	glEnable( GL_DEPTH_TEST );

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	memset( m_boundSamplers, 0, sizeof( m_boundSamplers ) );
	memset( m_srgbDecode, 0, sizeof( m_srgbDecode ) );

#ifdef _WIN32

	std::vector<cl_platform_id> platforms;
	cl_uint platformCount = 0;
	clGetPlatformIDs( 1000, nullptr, &platformCount );
	platforms.resize( platformCount );
	clGetPlatformIDs( platformCount, &platforms[0], nullptr );

	if( platformCount == 0 )
	{
		return E_FAIL;
	}

	typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetGLContextInfoKHR_fn)( 
		const cl_context_properties * properties, 
		cl_gl_context_info param_name,
		size_t param_value_size, 
		void * param_value, 
		size_t * param_value_size_ret );
	clGetGLContextInfoKHR_fn pclGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress( "clGetGLContextInfoKHR" );

	m_clContext = nullptr;
	m_clQueue = nullptr;
	for( cl_uint i = 0; i < platformCount; ++i )
	{
		cl_context_properties props[] =
		{
			CL_CONTEXT_PLATFORM, (cl_context_properties) platforms[i],
			CL_GL_CONTEXT_KHR, (cl_context_properties) m_hRC,
			CL_WGL_HDC_KHR, (cl_context_properties) m_hDC,
			0,
		};

		size_t deviceCount = 0;
		pclGetGLContextInfoKHR( props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, 0, nullptr, &deviceCount ); 
		if( deviceCount )
		{
			pclGetGLContextInfoKHR( props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof( m_clDevice ), &m_clDevice, nullptr ); 

			m_clContext = clCreateContext( props, 1, &m_clDevice, &OnClError, nullptr, nullptr );
			m_clQueue = clCreateCommandQueue( m_clContext, m_clDevice, CL_QUEUE_PROFILING_ENABLE, nullptr );
			break;
		}

	}
#else
    auto cglContext = CGLGetCurrentContext();
    auto shareGroup = CGLGetShareGroup( cglContext );
    
    cl_context_properties props[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)shareGroup,
        0 };
    m_clContext = clCreateContext( props, 0, nullptr, &OnClError, nullptr, nullptr );
    clGetContextInfo( m_clContext, CL_CONTEXT_DEVICES, sizeof( m_clDevice ), &m_clDevice, nullptr );
    m_clQueue = clCreateCommandQueue( m_clContext,m_clDevice,CL_QUEUE_PROFILING_ENABLE,NULL );
#endif
	if( !m_clContext || !m_clQueue || !m_clDevice )
	{
		return E_FAIL;
	}

	CCP_AL_LOG( "OpenCL device: %s %s %s %s\nDevice extensions: %s\n", 
		GetClString( clGetDeviceInfo, m_clDevice, CL_DEVICE_PROFILE ).c_str(),
		GetClString( clGetDeviceInfo, m_clDevice, CL_DEVICE_VERSION ).c_str(),
		GetClString( clGetDeviceInfo, m_clDevice, CL_DEVICE_NAME ).c_str(),
		GetClString( clGetDeviceInfo, m_clDevice, CL_DEVICE_VENDOR ).c_str(),
		GetClString( clGetDeviceInfo, m_clDevice, CL_DEVICE_EXTENSIONS ).c_str() );

	return S_OK;
}

PixelFormat Tr2RenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer.GetFormat();
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the current GPU is in AFR mode and returns the number of AFR groups. Works
//   for nVidia and ATI GPUs.
//   TODO: implement me
// Arguments:
//   count - (out) Number of AFR groups
// Return Value:
//   HRESULT of the call.
// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetAFRGroupCount( uint32_t& count )
{
	count = 1;
	return S_OK;
}

#if defined(_WIN32) && ( defined( TRINITYDEV ) || !defined( NDEBUG ) )

namespace
{

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	switch( type )
	{
	case GL_DEBUG_TYPE_ERROR_ARB:
		CCP_AL_LOGERR( "OpenGL Error: %s", message );
		OutputDebugString( "OpenGL Error: " );
		OutputDebugString( message );
		OutputDebugString( "\n" );
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
		CCP_AL_LOGWARN( "OpenGL Deprecated Warning: %s", message );
		OutputDebugString( "OpenGL Deprecated Warning: " );
		OutputDebugString( message );
		OutputDebugString( "\n" );
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
		CCP_AL_LOGWARN( "OpenGL Undefined Behavior: %s", message );
		OutputDebugString( "OpenGL Undefined Behavior: " );
		OutputDebugString( message );
		OutputDebugString( "\n" );
		break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB:
		CCP_AL_LOGWARN( "OpenGL Portability Issue: %s", message );
		OutputDebugString( "OpenGL Portability Issue: " );
		OutputDebugString( message );
		OutputDebugString( "\n" );
		break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB:
		CCP_AL_LOGWARN( "OpenGL Performance Issue: %s", message );
		OutputDebugString( "OpenGL Performance Issue: " );
		OutputDebugString( message );
		OutputDebugString( "\n" );
		break;
	default:
		// CCP_AL_LOG( "OpenGL: %s", message );
		// OutputDebugString( "OpenGL: " );
		// OutputDebugString( message );
		// OutputDebugString( "\n" );
		break;
	}
}

}

#endif

ALResult Tr2RenderContextAL::CreateOpenGLContext( Tr2PresentParametersAL& pp )
{
	m_hWnd = (Tr2WindowHandle)pp.outputWindow;
#ifdef _WIN32
	m_hDC = GetDC( m_hWnd );

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iFormat = ChoosePixelFormat( m_hDC, &pfd );
	SetPixelFormat( m_hDC, iFormat, &pfd );

	int flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#if defined( TRINITYDEV ) || !defined( NDEBUG )
	flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

	int attriblist[] = 
	{ 
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4, 
		WGL_CONTEXT_MINOR_VERSION_ARB, 1, 
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 
		WGL_CONTEXT_FLAGS_ARB, flags,
		0
	};

	auto tempCtx = wglCreateContext( m_hDC );
	wglMakeCurrent( m_hDC, tempCtx );

	auto ret = glewInit();
	CCP_AL_LOG( "glewInit %d", ret );

	auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress( "wglCreateContextAttribsARB" );
	wglDeleteContext( tempCtx );
	
	m_hRC = wglCreateContextAttribsARB( m_hDC, nullptr, attriblist );
	wglMakeCurrent( m_hDC, m_hRC );

#if defined( TRINITYDEV ) || !defined( NDEBUG )
	if( GLEW_ARB_debug_output )
	{
		glDebugMessageCallbackARB( &DebugCallback, nullptr );
	}
#endif

#elif !defined(__APPLE__)
	auto ret = glewInit();
	CCP_AL_LOG( "glewInit %d", ret );
#endif
	const char* version = reinterpret_cast<const char*>( glGetString( GL_VERSION ) );
	CCP_AL_LOG( "OpenGL version: %s", version );
    
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
    glfwSwapInterval( pp.presentInterval & 0xf );
#endif

	CR_RETURN_HR( m_defaultBackBuffer.Create(	
		presentationParameters.mode.width,
		presentationParameters.mode.height,
		1,
		PIXEL_FORMAT_B8G8R8A8_UNORM,
		1,
		0,
		*this ) );
	CR_RETURN_HR( m_defaultDepthStencil.Create(
		presentationParameters.mode.width,
		presentationParameters.mode.height,
		DSFMT_D24S8,									// <- ?
		1,
		0,
		*this ) );

	SetRenderTarget( m_defaultBackBuffer );
	SetDepthStencil( m_defaultDepthStencil );
	
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

	AL_UPDATE_RESOURCE_FRAME_USAGE( source );
	GL_FAIL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
	ON_BLOCK_EXIT( [&] { SetRtDsToDevice( 0 ); } );
	GL_FAIL( glViewport( 0, 0, source.GetWidth(), source.GetHeight() ) );

	if( !m_blitter )
	{
		m_blitter = new Blitter();
		CR_RETURN_HR( m_blitter->Create( *this ) );
	}
	ALResult result = m_blitter->Blit( source );

	glBindProgramPipeline( m_pipeline );

	m_currentActiveTexture = 0;
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
#if AL_TACK_RESOURCE_USAGE && TRACK_AL_RESOURCES
	extern uint64_t g_trackCurrentFrame;
	++g_trackCurrentFrame;
#endif

	InternalBlitToBackBuffer( m_defaultBackBuffer.GetTexture() );

#ifdef _WIN32
	if( !SwapBuffers( m_hDC ) )
	{
		return E_FAIL;
	}
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
	AL_UPDATE_RESOURCE_FRAME_USAGE( layout );
	m_boundLayout = layout.m_definition.get();
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShader( const Tr2ShaderAL& shader )
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( shader );
	switch( shader.GetType() )
	{
	case VERTEX_SHADER:
		m_vertexShader = &shader;
		return S_OK;
	case PIXEL_SHADER:
		m_pixelShader = &shader;
		return S_OK;
	case COMPUTE_SHADER:
		m_computeShader = &shader;
		return S_OK;
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2RenderContextAL::SetProgram()
{
	if( !m_vertexShader || !m_vertexShader->IsValid() || 
		!m_pixelShader || !m_pixelShader->IsValid() )
	{
		return E_FAIL;
	}

	bool patchedPS = (m_alphaTestParameters.m_alphaTestEnabled && 
		m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS) ||
		m_fragmentOpSettings.m_clipPlaneEnable;

	GL_FAIL( glUseProgramStages( 
		m_pipeline, 
		GL_VERTEX_SHADER_BIT, 
		patchedPS && m_vertexShader->m_patchedShader.shader ? m_vertexShader->m_patchedShader.shader : m_vertexShader->m_shader.shader ) );
	GL_FAIL( glUseProgramStages( 
		m_pipeline, 
		GL_FRAGMENT_SHADER_BIT, 
		patchedPS && m_pixelShader->m_patchedShader.shader ? m_pixelShader->m_patchedShader.shader : m_pixelShader->m_shader.shader ) );

	return S_OK;
}

void Tr2RenderContextAL::ShaderDeleted( int shader )
{
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
		if( value )
		{
			glEnable( GL_CLIP_DISTANCE0 );
		}
		else
		{
			glDisable( GL_CLIP_DISTANCE0 );
		}
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
	case RS_SCISSORTESTENABLE:
		if( value )
		{
			GL_FAIL( glEnable( GL_SCISSOR_TEST ) );
		}
		else
		{
			GL_FAIL( glDisable( GL_SCISSOR_TEST ) );
		}
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
			GL_FAIL( glEnable( GL_FRAMEBUFFER_SRGB ) );
		}
		else
		{
			GL_FAIL( glDisable( GL_FRAMEBUFFER_SRGB ) );
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

ALResult Tr2RenderContextAL::SetClipPlane( uint32_t planeIndex, const float* planeEq )
{
	m_fragmentOpSettings.SetClipPlane( planeIndex, planeEq );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetScissorRect(	
	uint32_t left, 
	uint32_t top, 
	uint32_t right, 
	uint32_t bottom )
{
	GL_FAIL( glScissor( left, top, int( right ) - int( left ), int( bottom ) - int( top ) ) );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderBuffer(		
	ShaderType inputType, 
	uint32_t slot, 
	const Tr2GpuBufferAL& buffer )
{
	if( inputType == COMPUTE_SHADER )
	{
		cl_mem mem = nullptr;
		if( !buffer.IsValid() )
		{
			if( &buffer != &nullGB )
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
			if( !buffer.m_clObject )
			{
				int error;
				buffer.m_clObject = clCreateFromGLBuffer( m_clContext, CL_MEM_READ_WRITE, *buffer.m_buffer, &error );
				if( !buffer.m_clObject )
				{
					return E_FAIL;
				}
			}
			mem = buffer.m_clObject;
		}
		if( m_boundTextures[slot] )
		{
			clReleaseMemObject( m_boundTextures[slot] );
		}
		m_boundTextures[slot] = mem;
		if( mem )
		{
			clRetainMemObject( mem );
		}
		return S_OK;
	}
	return E_FAIL;
}

ALResult Tr2RenderContextAL::SetUav(
	Tr2RenderContextEnum::ShaderType inputType, 
	uint32_t slot, 
	const Tr2GpuBufferAL& buffer,
	uint32_t initialCount ) throw()
{
	if( inputType != COMPUTE_SHADER || slot > 16 )
	{
		return E_INVALIDARG;
	}
	cl_mem mem = nullptr;
	if( !buffer.IsValid() )
	{
		if( &buffer != &nullGB )
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
		if( !buffer.m_clObject )
		{
			int error;
			buffer.m_clObject = clCreateFromGLBuffer( m_clContext, CL_MEM_READ_WRITE, *buffer.m_buffer, &error );
			if( !buffer.m_clObject )
			{
				return E_FAIL;
			}
		}
		mem = buffer.m_clObject;
	}
	if( m_boundUavs[slot] )
	{
		clReleaseMemObject( m_boundUavs[slot] );
	}
	m_boundUavs[slot] = mem;
	if( mem )
	{
		clRetainMemObject( mem );
	}
	return S_OK;
}


ALResult Tr2RenderContextAL::SetTexture(	
	ShaderType inputType, 
	uint32_t slot, 
	const Tr2TextureAL& texture, 
	Tr2RenderContextEnum::ColorSpace colorSpace )
{
	if( slot >= 16 )
	{
		return E_INVALIDARG;
	}
	if( inputType == COMPUTE_SHADER )
	{
		cl_mem mem = nullptr;
		if( !texture.IsValid() )
		{
			if( &texture != &nullTX )
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			AL_UPDATE_RESOURCE_FRAME_USAGE( texture );
			if( !texture.m_clObject )
			{
				switch( texture.GetType() )
				{
				case TEX_TYPE_1D:
				case TEX_TYPE_2D:
#ifdef _WIN32
					texture.m_clObject = clCreateFromGLTexture2D( m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, *texture.m_texture, nullptr );
#else
					texture.m_clObject = clCreateFromGLTexture( m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, *texture.m_texture, nullptr );
#endif
					break;
				case TEX_TYPE_3D:
#ifdef _WIN32
					texture.m_clObject = clCreateFromGLTexture3D( m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, *texture.m_texture, nullptr );
#else
					texture.m_clObject = clCreateFromGLTexture( m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, *texture.m_texture, nullptr );
#endif
					break;
				default:
					return E_FAIL;
				}
				if( !texture.m_clObject )
				{
					return E_FAIL;
				}
			}
			mem = texture.m_clObject;
		}
		if( m_boundTextures[slot] )
		{
			clReleaseMemObject( m_boundTextures[slot] );
		}
		m_boundTextures[slot] = mem;
		if( mem )
		{
			clRetainMemObject( mem );
		}
		return S_OK;
	}


	if( !texture.IsValid() )
	{
		return &texture == &nullTX ? S_OK : E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( texture );
	// TODO: handle sRGB: like in DX11 with texture.m_texture[2]?
	if( m_currentActiveTexture != slot )
	{
		GL_FAIL( glActiveTexture( GL_TEXTURE0 + slot ) );	//TODO vertex shader textures
		m_currentActiveTexture = slot;
	}
	GL_FAIL( glBindTexture( ConvertTextureType( texture.GetType() ), *texture.m_texture ) );
	m_srgbDecode[inputType][slot] = colorSpace == COLOR_SPACE_SRGB;
	if( m_boundSamplers[inputType][slot] )
	{
		GL_FAIL( glSamplerParameteri( m_boundSamplers[inputType][slot],
									GL_TEXTURE_SRGB_DECODE_EXT, 
									m_srgbDecode[inputType][slot] ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT  ) );
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
	CR_GL( glDepthRange( viewport.m_minZ, viewport.m_maxZ ) );
	return S_OK;
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport )
{
	viewport = m_currentViewport;
	return S_OK;
}

bool Tr2RenderContextAL::IsSupportedRenderTargetFormat(	PixelFormat /*format*/, 
														bool /*withAutoGenMipmap*/ )
{
	// TODO:
	return true;
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
	if( m_boundRenderTarget[slot] )
	{
		width = m_boundRenderTarget[slot]->GetWidth();
		height = m_boundRenderTarget[slot]->GetHeight();
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
		internalFormat = GL_RG32F;
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
		targetType   = GL_R11F_G11F_B10F;
		return true;

	case PIXEL_FORMAT_R8G8B8A8_TYPELESS:
	case PIXEL_FORMAT_R8G8B8A8_UNORM:
	case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
		internalFormat = GL_SRGB8_ALPHA8;
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
		internalFormat = GL_R32F;
		targetFormat = GL_RED;
		targetType   = GL_FLOAT;
		return true;

	case PIXEL_FORMAT_R32_UINT:
		internalFormat = GL_R32UI;
		targetFormat = GL_RED;
		targetType   = GL_UNSIGNED_INT;
		return true;

	case PIXEL_FORMAT_R32_SINT:
		internalFormat = GL_R32I;
		targetFormat = GL_RED;
		targetType   = GL_INT;
		return true;

	case PIXEL_FORMAT_R8G8_TYPELESS:
	case PIXEL_FORMAT_R8G8_UNORM:
		internalFormat = GL_RG8;
		targetFormat = GL_RG;
		targetType   = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8G8_UINT:
		internalFormat = GL_RG8UI;
		targetFormat = GL_RG;
		targetType   = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8G8_SNORM:
		internalFormat = GL_RG8_SNORM;
		targetFormat = GL_RG;
		targetType   = GL_BYTE;
		return true;

	case PIXEL_FORMAT_R8G8_SINT:
		internalFormat = GL_RG8I;
		targetFormat = GL_RG;
		targetType   = GL_BYTE;
		return true;

	case PIXEL_FORMAT_R16_TYPELESS:
	case PIXEL_FORMAT_R16_FLOAT:
		internalFormat = GL_R16F;
		targetFormat = GL_RED;
		targetType   = GL_HALF_FLOAT;
		return true;

	case PIXEL_FORMAT_R16_UNORM:
		internalFormat = GL_R16;
		targetFormat = GL_RED;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16_UINT:
		internalFormat = GL_R16UI;
		targetFormat = GL_RED;
		targetType   = GL_UNSIGNED_SHORT;
		return true;

	case PIXEL_FORMAT_R16_SNORM:
		internalFormat = GL_R16_SNORM;
		targetFormat = GL_RED;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R16_SINT:
		internalFormat = GL_R16I;
		targetFormat = GL_RED;
		targetType   = GL_SHORT;
		return true;

	case PIXEL_FORMAT_R8_TYPELESS:
	case PIXEL_FORMAT_R8_UNORM:
		internalFormat = GL_R8;
		targetFormat = GL_RED;
		targetType = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8_UINT:
		internalFormat = GL_R8UI;
		targetFormat = GL_RED;
		targetType = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_R8_SNORM:
		internalFormat = GL_R8_SNORM;
		targetFormat = GL_RED;
		targetType = GL_BYTE;
		return true;

	case PIXEL_FORMAT_R8_SINT:
		internalFormat = GL_R8I;
		targetFormat = GL_RED;
		targetType = GL_BYTE;
		return true;

	case PIXEL_FORMAT_A8_UNORM:
		internalFormat = GL_SRGB8_ALPHA8;
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
			
	case PIXEL_FORMAT_BC4_TYPELESS:
	case PIXEL_FORMAT_BC4_UNORM:
		internalFormat = GL_COMPRESSED_RED_RGTC1;
		targetFormat = GL_COMPRESSED_RED_RGTC1;
		targetType   = 0;
		return true;
			
	case PIXEL_FORMAT_BC5_TYPELESS:
	case PIXEL_FORMAT_BC5_UNORM:
		internalFormat = GL_COMPRESSED_RG_RGTC2;
		targetFormat = GL_COMPRESSED_RG_RGTC2;
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
		internalFormat = GL_SRGB8_ALPHA8;
		targetFormat = GL_BGRA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;

	case PIXEL_FORMAT_B8G8R8X8_UNORM:
	case PIXEL_FORMAT_B8G8R8X8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
		internalFormat = GL_SRGB8;
		targetFormat = GL_BGRA;
		targetType   = GL_UNSIGNED_BYTE;
		return true;
	default:
		return false;
	}
	
}

void Tr2RenderContextAL::ReleaseDeviceResources()
{
	delete m_blitter;
	m_blitter = nullptr;
}

bool Tr2RenderContextAL::ApplyVertexDeclaration( ShadowStateRestoreInfo& info )
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

	if( m_boundLayout == nullptr )
	{
		return false;
	}

	unsigned currentArray = 8;
	auto& definition = *m_boundLayout;
	info.m_instanceAttributesCount = 0;

	for( size_t i = 0; i != definition.m_items.size(); ++i )
    {
		const Tr2VertexDefinition::Item& src = definition.m_items[i];
		if( src.m_stream >= 8 )
		{
			return false;
		}
		auto it = m_vertexShader->m_inputs.find( std::make_pair( src.m_usage, src.m_usageIndex ) );
		if( it != m_vertexShader->m_inputs.end() )
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
			glEnableVertexAttribArray( it->second ); 

			uintptr_t offset = src.m_offset;
			glVertexAttribPointer( it->second, 
									definition.GetDataTypeSizeInMembers( src.m_dataType ),
									elemType[ src.m_dataType & ( Tr2VertexDefinition::DT_TYPE_MASK | Tr2VertexDefinition::DT_UNSIGNED_BIT ) ], 
									src.m_dataType & Tr2VertexDefinition::DT_NORMALIZED_BIT ? GL_TRUE : GL_FALSE,
									m_boundStreams[src.m_stream].stride, 
									reinterpret_cast<const GLvoid*>( offset ) );
			if( src.m_instanceStepRate )
			{
				glVertexAttribDivisorARB( it->second, 1 );
				info.m_instanceAttributes[info.m_instanceAttributesCount++] = it->second;
			}
		}
    }

	return true;
}

bool Tr2RenderContextAL::ApplyShadowRenderStates( ShadowStateRestoreInfo& info )
{
	m_fragmentOpSettings.UpdateContents( m_alphaTestParameters );
	CR_RETURN_VAL( SetProgram(), false );

	bool patchedPS = (m_alphaTestParameters.m_alphaTestEnabled && 
		m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS) ||
		m_fragmentOpSettings.m_clipPlaneEnable;

	const Tr2ShaderAL::GLShader& vs = patchedPS && m_vertexShader->m_patchedShader.shader ? m_vertexShader->m_patchedShader : m_vertexShader->m_shader;
	const Tr2ShaderAL::GLShader& ps = patchedPS && m_pixelShader->m_patchedShader.shader ? m_pixelShader->m_patchedShader : m_pixelShader->m_shader;
	for( unsigned i = 0; i != 15; ++i )
	{
		if( m_boundBuffers[Tr2RenderContextEnum::VERTEX_SHADER][i] && vs.constantBuffers[i] != -1 )
		{
			CR_GL_RETURN_VAL( glProgramUniform4fv( vs.shader, vs.constantBuffers[i],
				m_boundBuffers[Tr2RenderContextEnum::VERTEX_SHADER][i]->GetSize() / 4 / sizeof( float ), 
				(float*)m_boundBuffers[Tr2RenderContextEnum::VERTEX_SHADER][i]->m_shadowCopy.get() ), false );
		}
		if( m_boundBuffers[Tr2RenderContextEnum::PIXEL_SHADER][i] && ps.constantBuffers[i] != -1 )
		{
			CR_GL_RETURN_VAL( glProgramUniform4fv( ps.shader, ps.constantBuffers[i],
				m_boundBuffers[Tr2RenderContextEnum::PIXEL_SHADER][i]->GetSize() / 4 / sizeof( float ), 
				(float*)m_boundBuffers[Tr2RenderContextEnum::PIXEL_SHADER][i]->m_shadowCopy.get() ), false );
		}
	}

	if( ps.constantBuffers[15] != -1 )
	{
		float psShadowState[] = {
				float( m_fragmentOpSettings.m_invertedAlphaTest ),
				float( m_fragmentOpSettings.m_alphaTestRef ),
				float( m_fragmentOpSettings.m_alphaTestFunc ),
				float( m_alphaTestParameters.m_alphaTestEnabled ),
				float( m_numberOfLights ),
				0,0,0 };
		CR_GL_RETURN_VAL( glProgramUniform4fv( ps.shader, ps.constantBuffers[15],
			2, 
			psShadowState ), false );
	}
	if( vs.constantBuffers[15] != -1 )
	{
		float vsShadowState[] = {
				1.0f / float( m_currentViewport.m_width ),
				-1.0f / float( m_currentViewport.m_height ),
				-1.f,
				0,
				0, 0, 0, 0,
		};
		if( m_fragmentOpSettings.m_clipPlaneEnable & 1 )
		{
			vsShadowState[4] = m_fragmentOpSettings.m_clipPlane[0][0];
			vsShadowState[5] = m_fragmentOpSettings.m_clipPlane[0][1];
			vsShadowState[6] = m_fragmentOpSettings.m_clipPlane[0][2];
			vsShadowState[7] = m_fragmentOpSettings.m_clipPlane[0][3];
		}
		CR_GL_RETURN_VAL( glProgramUniform4fv( vs.shader, vs.constantBuffers[15],
			2, 
			vsShadowState ), false );
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
		if( m_boundRenderTarget[i] )
		{
			AL_UPDATE_RESOURCE_FRAME_USAGE( *m_boundRenderTarget[i] );
			if( m_boundRenderTarget[i]->m_msaaTarget )
			{
				colorBuffer[i] = m_boundRenderTarget[i]->m_msaaTarget;
				colorBufferType[i] = GL_RENDERBUFFER;
			}
			else
			{
				colorBuffer[i] = *m_boundRenderTarget[i]->m_backingStore.m_texture;
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

	if( m_boundDepthStencil && m_renderStates[RS_ZENABLE] )
	{
		glEnable( GL_DEPTH_TEST );
	}
	else
	{
		glDisable( GL_DEPTH_TEST );
	}

	const Tr2RenderTargetAL& bb = m_boundRenderTarget[0] ? *m_boundRenderTarget[0] : m_defaultBackBuffer;

	// Msaa zero is the same as one, so does need to show up as 'compatible'
	const uint32_t dsMsaaType = m_boundDepthStencil ? std::max( m_boundDepthStencil->GetMsaaType(), 1u ) : 1;
	const uint32_t bbMsaaType = std::max( bb.GetMsaaType(), 1u );

	// dont't even bother setting it when the dimensions don't match, it's not gonna work.
	// This happens when we set/push/pop an RT and DS in two separate calls -- there's a point between
	// those two where it's in a bad state.  Silently "works" in DX9, complains in DX11. Fix the spam:
	if( !m_boundDepthStencil	||
		( m_boundDepthStencil->GetWidth()		== bb.GetWidth()		&&
		  m_boundDepthStencil->GetHeight()		== bb.GetHeight()		&&
		  m_boundDepthStencil->GetMsaaQuality() == bb.GetMsaaQuality()	&&
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
		if( m_boundDepthStencil && m_boundDepthStencil->IsValid() )
		{
			AL_UPDATE_RESOURCE_FRAME_USAGE( *m_boundDepthStencil );
			if( m_boundDepthStencil->IsReadable() )
			{
				GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *m_boundDepthStencil->m_readableDepth.m_texture, 0 ) );
				GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, *m_boundDepthStencil->m_readableDepth.m_texture, 0 ) );
			}
			else
			{
				GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_boundDepthStencil->m_depthRenderBuffer ) );
				GL_FAIL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_boundDepthStencil->m_depthRenderBuffer ) );
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
		SetScissorRect( 0, 0, bb.GetWidth(), bb.GetHeight() );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::CopySubresourceRegion( 
	Tr2TextureAL& destination,
	const Tr2TextureSubresource& destSubresource,
	Tr2TextureAL& source, 
	const Tr2TextureSubresource& sourceSubresource )
{
	if( !destination.IsValid() || !source.IsValid() || !IsValid() )
	{
		return E_FAIL;
	}


	Tr2TextureSubresource src = sourceSubresource;
	Tr2TextureSubresource dst = destSubresource;	
	
	if( !Crop( src, source, dst, destination ) )
	{
		return E_INVALIDARG;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( source );
	AL_UPDATE_RESOURCE_FRAME_USAGE( destination );
	uint32_t box[6] = { src.m_left, src.m_top, src.m_front, src.m_right, src.m_bottom, src.m_back };

	const uint32_t srcMipCount = source.GetTrueMipCount();
	const uint32_t dstMipCount = destination.GetTrueMipCount();

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
			GLenum srcTexType = source.GetType() == TEX_TYPE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_TEXTURE_2D;
			GL_FAIL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, srcTexType, *source.m_texture, src.m_startMipLevel + mip ) );

			//SetViewport( Tr2Viewport ( source.GetMipWidth( src.m_startMipLevel + mip ), source.GetMipHeight( src.m_startMipLevel + mip ) ) );
			//SetScissorRect( 0, 0, source.GetMipWidth( src.m_startMipLevel + mip ), source.GetMipHeight( src.m_startMipLevel + mip ) );

			GLenum texType = destination.GetType() == TEX_TYPE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_TEXTURE_2D;
			uint32_t height = destination.GetMipHeight( dst.m_startMipLevel + mip );
			GL_FAIL( glBindTexture( texType, *destination.m_texture ) );
			GL_FAIL( glCopyTexSubImage2D( texType, dst.m_startMipLevel + mip, dst.m_left, dst.m_top, box[0], height - box[4], box[3] - box[0], box[4] - box[1] ) );
		}

		if( mip + 1 != src.GetMipCount() )
		{
			AdvanceMip( src, source, mip );
			AdvanceMip( dst, destination,  mip );
		}

		for( uint32_t k = 0; k < 6; ++k )
		{
			box[k] = box[k] >> 1;
		}
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::InternalResolveRT( Tr2RenderTargetAL& destination, const Tr2RenderTargetAL& source )
{
	if( !IsValid() || !destination.IsValid() || !source.IsValid() || !destination.m_backingStore.IsValid() )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( source );
	AL_UPDATE_RESOURCE_FRAME_USAGE( destination );
	CR_GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
	CR_GL( glBindFramebuffer( GL_READ_FRAMEBUFFER, m_offscreenFrameBuffer0 ) );
	if( source.m_msaaTarget )
	{
		CR_GL( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, source.m_msaaTarget ) );
	}
	else
	{
		CR_GL( glFramebufferTexture2D( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *source.m_backingStore.m_texture, 0 ) );
	}
#if defined(TRINITYDEV) || defined(_DEBUG)
	if( glCheckFramebufferStatus( GL_READ_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		CCP_AL_LOGERR( "Tr2RenderContextAL::InternalResolveRT: invalid read buffer" );
		return E_FAIL;
	}
#endif
	CR_GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_offscreenFrameBuffer1 ) );
	CR_GL( glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *destination.m_backingStore.m_texture, 0 ) );
#if defined(TRINITYDEV) || defined(_DEBUG)
	if( glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		CCP_AL_LOGERR( "Tr2RenderContextAL::InternalResolveRT: invalid draw buffer" );
		return E_FAIL;
	}
#endif
	CR_GL( glBlitFramebuffer( 0, 0, source.GetWidth(), source.GetHeight(), 0, 0, destination.GetWidth(), destination.GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
	CR_GL( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
	CR_GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
	CR_GL( glBindFramebuffer( GL_FRAMEBUFFER, m_defaultFrameBufferObject ) ); 
	return S_OK;
}

#endif
