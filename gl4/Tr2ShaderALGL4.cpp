#include "StdAfx.h"
#include "Tr2ShaderALGL4.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ShaderAL::Tr2ShaderAL()
	: m_type( INVALID_SHADER ),
	m_bytecode( "Tr2ShaderAL::m_byteCode" ),
	m_clKernel( nullptr )
{
	m_shader.shader = 0;
	m_patchedShader.shader = 0;
}

static int CreateShader( Tr2RenderContextEnum::ShaderType type, const char* prefix, const void* bytecode, uint32_t bytecodeSize )
{
#pragma warning( disable: 4189 )	// scopeguard

	GLenum shaderType;
	GLuint shader = 0;
	switch( type )
	{
	case VERTEX_SHADER:
		shaderType = GL_VERTEX_SHADER;
		break;
	case PIXEL_SHADER:
		shaderType = GL_FRAGMENT_SHADER;
		break;
	default:
		CCP_AL_LOGERR( "Tr2ShaderAL: invalid shader type" );
		return 0;
	}
	ON_BLOCK_EXIT( [&]{ if( shader ) { glDeleteProgram( shader ); } } );

	static const char* version = "#version 410 core\n";

	const char* code[] = { version, prefix, reinterpret_cast<const char*>( bytecode ) };
	shader = glCreateShaderProgramv( shaderType, 3, code );
	if( !shader )
	{
		CCP_AL_LOGERR( "Tr2ShaderAL: shader creation failed" );
		return 0;
	}

	GLint status = 0;
	glGetProgramiv( shader, GL_LINK_STATUS, &status );
	if( !status )
	{
		GLint length;
		glGetProgramiv( shader, GL_INFO_LOG_LENGTH, &length );
		char* buffer = new char[length];
		glGetProgramInfoLog( shader, length, nullptr, buffer );
		CCP_AL_LOGERR( "Tr2ShaderAL: error compiling shader: %s", buffer );
		delete[] buffer;
		return 0;
	}
#if defined( TRINITYDEV ) || !defined( NDEBUG )
	else
	{
		GLint length;
		glGetProgramiv( shader, GL_INFO_LOG_LENGTH, &length );
		if( length )
		{
			char* buffer = new char[length];
			glGetProgramInfoLog( shader, length, nullptr, buffer );
			CCP_AL_LOGWARN( "Tr2ShaderAL: warnings compiling shader: %s", buffer );
			delete[] buffer;
		}
	}
#endif
	int result = shader;
	shader = 0;
	return result;
}

ALResult Tr2ShaderAL::Create( 
	Tr2RenderContextAL& renderContext, 
	Tr2RenderContextEnum::ShaderType type, 
	const void* bytecode, 
	uint32_t bytecodeSize, 
	const void* /*patchedBytecode*/, 
	uint32_t /*patchedBytecodeSize*/, 
	const Tr2ShaderInputDefinition& inputDefinition )
{
	Destroy();

	if( type == COMPUTE_SHADER )
	{
		return CreateCL( bytecode, bytecodeSize, inputDefinition, renderContext );
	}
	m_type = type;

	int shader = CreateShader( type, "", bytecode, bytecodeSize );
	if( shader == 0 )
	{
		m_type = INVALID_SHADER;
		return E_FAIL;
	}
	m_shader.shader = shader;

	shader = CreateShader( type, "#define PS\n", bytecode, bytecodeSize );
	if( shader == 0 )
	{
		glDeleteProgram( m_shader.shader );
		m_shader.shader = 0;
		m_type = INVALID_SHADER;
		return E_FAIL;
	}
	m_patchedShader.shader = shader;
	m_bytecode.resize( bytecodeSize );
	if( !m_bytecode.empty() )
	{
		memcpy( &m_bytecode[0], bytecode, bytecodeSize );
	}
	m_inputDefinition = inputDefinition;
	for( auto it = inputDefinition.elements.begin(); it != inputDefinition.elements.end(); ++it )
	{
		m_inputs[std::make_pair( it->usage, it->usageIndex )] = it->registerIndex;
	}
	static const char* cbNames[] = {
		"cb0",  "cb1",  "cb2",  "cb3",
		"cb4",  "cb5",  "cb6",  "cb7",
		"cb8",  "cb9",  "cb10", "cb11",
		"cb12", "cb13", "cb14", "ss",		
	};

	for( unsigned i = 0; i < 16; ++i )
	{
		m_shader.constantBuffers[i] = glGetUniformLocation( m_shader.shader, cbNames[i] );
		m_patchedShader.constantBuffers[i] = glGetUniformLocation( m_patchedShader.shader, cbNames[i] );
	}

	static const char* samplerNames[] = {
		"t0",  "t1",  "t2",  "t3",
		"t4",  "t5",  "t6",  "t7",
		"t8",  "t9",  "t10", "t11",
		"t12", "t13", "t14", "t15",		
	};

	for( unsigned i = 0; i < 16; ++i )
	{
		GLint location = glGetUniformLocation( m_shader.shader, samplerNames[i] );
		if( location != -1 )
		{
			glProgramUniform1i( m_shader.shader, location, i );
		}
		location = glGetUniformLocation( m_patchedShader.shader, samplerNames[i] );
		if( location != -1 )
		{
			glProgramUniform1i( m_patchedShader.shader, location, i );
		}
	}

	ChangeObjectId();

	return S_OK;
}

Tr2ShaderAL::~Tr2ShaderAL()
{
	Destroy();
}

void Tr2ShaderAL::Destroy()
{
	if( m_shader.shader )
	{
		glDeleteProgram( m_shader.shader );
		Tr2RenderContextAL::ShaderDeleted( m_shader.shader );
		m_shader.shader = 0;
	}
	if( m_patchedShader.shader )
	{
		glDeleteProgram( m_patchedShader.shader );
		Tr2RenderContextAL::ShaderDeleted( m_patchedShader.shader );
		m_patchedShader.shader = 0;
	}
	if( m_clKernel )
	{
		clReleaseKernel( m_clKernel );
		m_clKernel = nullptr;
	}
	m_type = INVALID_SHADER;
	m_inputs.clear();
}

ALResult Tr2ShaderAL::CreateCL( const void* bytecode, uint32_t bytecodeSize, const Tr2ShaderInputDefinition& inputDefinition, Tr2RenderContextAL& renderContext )
{
    size_t size = size_t( bytecodeSize );
	cl_program program = clCreateProgramWithSource(renderContext.m_clContext, 1, (const char**)&bytecode, &size, nullptr);
	if( !program )
	{
		return E_FAIL;
	}
	if( clBuildProgram( program, 1, &renderContext.m_clDevice, "", nullptr, nullptr ) != CL_SUCCESS ) 
	{
		size_t length;
		clGetProgramBuildInfo( program, renderContext.m_clDevice, CL_PROGRAM_BUILD_LOG, 0, nullptr, &length );
		if( length )
		{
			std::unique_ptr<char[]> buffer( new char[length] );
			clGetProgramBuildInfo(program, renderContext.m_clDevice, CL_PROGRAM_BUILD_LOG, length, buffer.get(), nullptr);
			CCP_AL_LOGERR( "Tr2ShaderAL: errors compiling shader: %s", buffer.get() );
		}
		return E_FAIL;
	}
	auto kernel = clCreateKernel( program, "cs", nullptr );
	clReleaseProgram( program );
	if( !kernel )
	{
		return E_FAIL;
	}
	m_type = COMPUTE_SHADER;
	m_bytecode.resize( bytecodeSize );
	if( !m_bytecode.empty() )
	{
		memcpy( &m_bytecode[0], bytecode, bytecodeSize );
	}
	m_clKernel = kernel;
	m_inputDefinition = inputDefinition;
	return S_OK;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != INVALID_SHADER && ( m_shader.shader != 0 || m_clKernel );
}

bool Tr2ShaderAL::operator==( const Tr2ShaderAL& shader ) const
{
	return m_shader.shader == shader.m_shader.shader && m_clKernel == shader.m_clKernel;
}

Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
{
	return m_type;
}

ALResult Tr2ShaderAL::GetBytecode( const void*& bytecode, uint32_t& size ) const
{
	if( m_type == INVALID_SHADER )
	{
		bytecode = nullptr;
		size = 0;
		return E_FAIL;
	}
	bytecode = m_bytecode.empty() ? nullptr : &m_bytecode[0];
	size = static_cast<uint32_t>( m_bytecode.size() );
	return S_OK;
}

const Tr2ShaderInputDefinition& Tr2ShaderAL::GetInputDefinition() const	
{ 
	return m_inputDefinition; 
}

void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
{
	CCP_ASSERT( !IsValid() );
	m_type = type;
}

#endif
