#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_OPENGLES2

#include "Tr2ShaderALGLES2.h"
#include "Tr2RenderContextGLES2.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

namespace
{
	int CreateShader( Tr2RenderContextEnum::ShaderType type, const Tr2ShaderBytecodeAL& bytecode )
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
		ON_BLOCK_EXIT( [&] { if( shader ) { glDeleteShader( shader ); } } );
		GLint length = GLint( bytecode.size );
		CR_GL_RETURN_VAL( glShaderSource( shader, 1, (const char**)&bytecode.bytecode, &length ), 0 );
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

}

namespace TrinityALImpl
{

	Tr2ShaderAL::Tr2ShaderAL()
		: m_type( INVALID_SHADER )
		, m_bytecode( "Tr2ShaderAL::m_byteCode" )
		, m_shader( 0 )
		, m_patchedShader( 0 )
	{
	}

	ALResult Tr2ShaderAL::Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderBytecodeAL&,
		const Tr2ShaderSignatureAL& signature,
		Tr2PrimaryRenderContextAL& )
	{
		Destroy();

		m_type = type;

		int shader = CreateShader( type, bytecode );
		if( shader == 0 )
		{
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		m_shader = shader;

		std::string patchedCode = "#define PS\n";
		patchedCode += std::string( (const char*)bytecode.bytecode, bytecode.size );

		shader = CreateShader( type, Tr2ShaderBytecodeAL( patchedCode.c_str(), patchedCode.length() ) );
		if( shader == 0 )
		{
			glDeleteShader( shader );
			m_shader = 0;
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		m_patchedShader = shader;
		m_bytecode.resize( bytecode.size );
		if( !m_bytecode.empty() )
		{
			memcpy( &m_bytecode[0], bytecode.bytecode, bytecode.size );
		}
		m_signature = signature;

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

	Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
	{
		return m_type;
	}

	ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
	{
		if( m_type == INVALID_SHADER )
		{
			bytecode = Tr2ShaderBytecodeAL();
			return E_FAIL;
		}
		bytecode.bytecode = m_bytecode.empty() ? nullptr : &m_bytecode[0];
		bytecode.size = m_bytecode.size();
		return S_OK;
	}

	void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderAL";
		description["shader"] = std::to_string( (long long)( GetType() ) );
		description["size"] = std::to_string( (long long)( m_bytecode.size() ) );
	}

}
#endif
