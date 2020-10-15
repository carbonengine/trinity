#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_OPENGLES2

#include "Tr2ShaderProgramALGLES2.h"
#include "Tr2ShaderALGLES2.h"
#include "Tr2RenderContextGLES2.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

	Tr2ShaderProgramAL::Tr2ShaderProgramAL()
		:m_isValid( false )
	{
		m_program.program = 0;
		m_patchedProgram.program = 0;
	}

	ALResult Tr2ShaderProgramAL::Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		if( count != 2 )
		{
			return E_INVALIDARG;
		}

		uint32_t mask = 0;
		for( size_t i = 0; i < count; ++i )
		{
			if( !shaders[i].IsValid() )
			{
				return E_INVALIDARG;
			}
			uint32_t bit = 1 << shaders[i].GetType();
			if( ( mask & bit ) != 0 )
			{
				return E_INVALIDARG;
			}
			mask |= bit;
		}
		if( mask != ( ( 1 << VERTEX_SHADER ) | ( 1 << PIXEL_SHADER ) ) )
		{
			return E_INVALIDARG;
		}

		if( shaders[0].GetType() == VERTEX_SHADER )
		{
			m_vertexShader = shaders[0];
			m_pixelShader = shaders[1];
		}
		else
		{
			m_vertexShader = shaders[1];
			m_pixelShader = shaders[0];
		}

		if( FAILED( CreateProgram( m_program, m_vertexShader.m_shader->m_shader, m_pixelShader.m_shader->m_shader, false ) ) )
		{
			Destroy();
			return E_FAIL;
		}

		if( m_vertexShader.m_shader->m_patchedShader || m_pixelShader.m_shader->m_patchedShader )
		{
			if( FAILED( CreateProgram(
				m_patchedProgram,
				m_vertexShader.m_shader->m_patchedShader ? m_vertexShader.m_shader->m_patchedShader : m_vertexShader.m_shader->m_shader,
				m_pixelShader.m_shader->m_patchedShader ? m_pixelShader.m_shader->m_patchedShader : m_pixelShader.m_shader->m_shader,
				true ) ) )
			{
				Destroy();
				return E_FAIL;
			}
		}
		m_registerMap = Tr2RegisterMapAL( shaders, count );

		m_isValid = true;
		return S_OK;
	}

	void Tr2ShaderProgramAL::Destroy()
	{
		m_vertexShader = ::Tr2ShaderAL();
		m_pixelShader = ::Tr2ShaderAL();
		if( m_program.program )
		{
			glDeleteProgram( m_program.program );
			m_program.program = 0;
			m_program.attributes.clear();
		}
		if( m_patchedProgram.program )
		{
			glDeleteProgram( m_patchedProgram.program );
			m_patchedProgram.program = 0;
			m_patchedProgram.attributes.clear();
		}
		m_registerMap = Tr2RegisterMapAL();
		m_isValid = false;
	}

	bool Tr2ShaderProgramAL::IsValid() const
	{
		return m_isValid;
	}

	Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	ALResult Tr2ShaderProgramAL::CreateProgram( ProgramData& program, GLuint vertexShader, GLuint pixelShader, bool useShadowStates )
	{
		program.program = glCreateProgram();
		if( program.program == 0 )
		{
			return E_FAIL;
		}
		GL_FAIL( glAttachShader( program.program, vertexShader ) );
		GL_FAIL( glAttachShader( program.program, pixelShader ) );

		glLinkProgram( program.program );
		GLint status = GL_FALSE;
		glGetProgramiv( program.program, GL_LINK_STATUS, &status );
		if( !status )
		{
			GLint length = 0;
			glGetProgramiv( program.program, GL_INFO_LOG_LENGTH, &length );
			std::unique_ptr<char[]> buffer( new char[length] );
			glGetProgramInfoLog( program.program, length, nullptr, buffer.get() );
			CCP_AL_LOGERR( "Error linking vertex and pixel shader: %s", buffer.get() );
			glDeleteProgram( program.program );
			program.program = 0;
			return E_FAIL;
		}
		static const char* attributeNames[] = {
			"attr0",  "attr1",  "attr2",  "attr3",
			"attr4",  "attr5",  "attr6",  "attr7",
			"attr8",  "attr9",  "attr10", "attr11",
			"attr12", "attr13", "attr14", "attr15",
		};

		program.attributes.clear();
		for( size_t i = 0; i < m_vertexShader.m_shader->m_signature.pipelineInputs.size(); ++i )
		{
			auto& element = m_vertexShader.m_shader->m_signature.pipelineInputs[i];
			int index = glGetAttribLocation( program.program, attributeNames[i] );
			if( index != -1 )
			{
				program.attributes[std::make_pair( element.usage, element.usageIndex )] = index;
			}
		}

		static const char* cbNames[] = {
			"cb0",  "cb1",  "cb2",  "cb3",
			"cb4",  "cb5",  "cb6",  "cb7",
			"cb8",  "cb9",  "cb10", "cb11",
			"cb12", "cb13", "cb14", "cb15",
		};

		for( unsigned i = 0; i < 16; ++i )
		{
			program.constantBuffers[i] = glGetUniformLocation( program.program, cbNames[i] );
		}
		program.intConstant = glGetUniformLocation( program.program, "i15" );

		if( useShadowStates )
		{
			program.shadowStateInt = glGetUniformLocation( program.program, "ssi" );
			program.shadowStateFloat = glGetUniformLocation( program.program, "ssf" );
		}
		else
		{
			program.shadowStateInt = -1;
			program.shadowStateFloat = -1;
		}
		program.shadowStateOffsets = glGetUniformLocation( program.program, "ssyf" );

		glUseProgram( program.program );

		static const char* samplerNames[] = {
			"s0",  "s1",  "s2",  "s3",
			"s4",  "s5",  "s6",  "s7",
			"s8",  "s9",  "s10", "s11",
			"s12", "s13", "s14", "s15",
		};

		for( unsigned i = 0; i < 16; ++i )
		{
			GLint location = glGetUniformLocation( program.program, samplerNames[i] );
			if( location != -1 )
			{
				glUniform1i( location, i );
			}
		}
		return S_OK;
	}

	void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderProgramAL";
	}

	const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
	{
		return m_registerMap;
	}
	}

#endif