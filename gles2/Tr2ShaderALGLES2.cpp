#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2ShaderALGLES2.h"
#include "Tr2RenderContextGLES2.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ShaderAL::Tr2ShaderAL()
	: m_type( INVALID_SHADER )
	, m_bytecode( "Tr2ShaderAL::m_byteCode" )
	, m_shader( 0 )
	, m_patchedShader( 0 )
{
}

static int CreateShader( Tr2RenderContextEnum::ShaderType type, const void* bytecode, uint32_t bytecodeSize )
{
#pragma warning( disable: 4189 )	// scopeguard

	int shader = 0;
	switch( type )
	{
	case VERTEX_SHADER:
		shader = glCreateShader( GL_VERTEX_SHADER );
		if( shader == 0 )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: vertex shader creation failed" );
			return 0;
		}
		break;
	case PIXEL_SHADER:
		shader = glCreateShader( GL_FRAGMENT_SHADER );
		if( shader == 0 )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: pixel shader creation failed" );
			return 0;
		}
		break;
	default:
		CCP_AL_LOGERR( "Tr2ShaderAL: invalid shader type" );
		return 0;
	}
	ON_BLOCK_EXIT( [&]{ if( shader ) { glDeleteShader( shader ); } } );
	GLint length = GLint( bytecodeSize );
	CR_GL_RETURN_VAL( glShaderSource( shader, 1, (const char**)&bytecode, &length ), 0 );
	CR_GL_RETURN_VAL( glCompileShader( shader ), 0 );
	GLint status = 0;
	CR_GL_RETURN_VAL( glGetShaderiv( shader, GL_COMPILE_STATUS, &status ), 0 );
	if( !status )
	{
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
		char* buffer = new char[length];
		glGetShaderInfoLog( shader, length, nullptr, buffer );
		CCP_AL_LOGERR( "Tr2ShaderAL: error compiling shader: %s", buffer );
		delete[] buffer;
		return 0;
	}
	int result = shader;
	shader = 0;
	return result;
}

ALResult Tr2ShaderAL::Create( 
	Tr2RenderContextAL& /*renderContext*/, 
	Tr2RenderContextEnum::ShaderType type, 
	const void* bytecode, 
	uint32_t bytecodeSize, 
	const void* /*patchedBytecode*/, 
	uint32_t /*patchedBytecodeSize*/, 
	const Tr2ShaderInputDefinition& inputDefinition )
{
	Destroy();

	m_type = type;

	int shader = CreateShader( type, bytecode, bytecodeSize );
	if( shader == 0 )
	{
		m_type = INVALID_SHADER;
		return E_FAIL;
	}
	m_shader = shader;

	std::string patchedCode = "#define PS\n";
	patchedCode += std::string( (const char*)bytecode, bytecodeSize );

	shader = CreateShader( type, patchedCode.c_str(), uint32_t( patchedCode.length() ) );
	if( shader == 0 )
	{
		glDeleteShader( shader );
		m_shader = 0;
		m_type = INVALID_SHADER;
		return E_FAIL;
	}
	m_patchedShader = shader;
	m_bytecode.resize( bytecodeSize );
	if( !m_bytecode.empty() )
	{
		memcpy( &m_bytecode[0], bytecode, bytecodeSize );
	}
	m_inputDefinition = inputDefinition;
	ChangeObjectId();

	return S_OK;
}

Tr2ShaderAL::~Tr2ShaderAL()
{
	Destroy();
}

void Tr2ShaderAL::Destroy()
{
	if( m_shader )
	{
		glDeleteShader( m_shader );
	}
	if( m_patchedShader )
	{
		glDeleteShader( m_patchedShader );
	}
	m_type = INVALID_SHADER;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != INVALID_SHADER && m_shader != 0;
}

bool Tr2ShaderAL::operator==( const Tr2ShaderAL& shader ) const
{
	return m_shader == shader.m_shader;
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
