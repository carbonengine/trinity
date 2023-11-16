#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2ShaderALDx11.h"
#include "ALLog.h"
#include "Tr2PrimaryRenderContextDx11.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
	Tr2ShaderAL::Tr2ShaderAL()
		: m_type( INVALID_SHADER ),
		m_pipelineInputHash( 0 )
	{
		m_shader.vertexShader = nullptr;
	}

	ALResult Tr2ShaderAL::Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderSignatureAL& signature,
		const char* shaderPath,
		Tr2PrimaryRenderContextAL &renderContext )
	{
		ReleaseShader();

		if( renderContext.m_d3dDevice11 == nullptr )
		{
			return E_FAIL;
		}

		m_type = type;

		m_bytecode.resize( "Tr2ShaderAL::m_byteCode", bytecode.size );
		if( !m_bytecode.empty() )
		{
			memcpy( m_bytecode.get(), bytecode.bytecode, bytecode.size );
		}

		m_signature = signature;

		m_shader.vertexShader = nullptr;

		switch( type )
		{
		case VERTEX_SHADER:
			if( FAILED( renderContext.m_d3dDevice11->CreateVertexShader( bytecode.bytecode, bytecode.size, nullptr, &m_shader.vertexShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: vertex shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		case PIXEL_SHADER:
			if( FAILED( renderContext.m_d3dDevice11->CreatePixelShader( bytecode.bytecode, bytecode.size, nullptr, &m_shader.pixelShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: pixel shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		case COMPUTE_SHADER:
			if( FAILED( renderContext.m_d3dDevice11->CreateComputeShader( bytecode.bytecode, bytecode.size, nullptr, &m_shader.computeShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: compute shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		case GEOMETRY_SHADER:
			if( FAILED( renderContext.m_d3dDevice11->CreateGeometryShader( bytecode.bytecode, bytecode.size, nullptr, &m_shader.geometryShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: geometry shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		case HULL_SHADER:
			if( FAILED( renderContext.m_d3dDevice11->CreateHullShader( bytecode.bytecode, bytecode.size, nullptr, &m_shader.hullShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: hull shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		case DOMAIN_SHADER:
			if( FAILED( renderContext.m_d3dDevice11->CreateDomainShader( bytecode.bytecode, bytecode.size, nullptr, &m_shader.domainShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: domain shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		default:
			CCP_AL_LOGERR( "Tr2ShaderAL: invalid shader type" );
			m_type = INVALID_SHADER;
			return E_INVALIDARG;
		}

		if( signature.pipelineInputs.empty() )
		{
			m_pipelineInputHash = 0;
		}
		else
		{
			m_pipelineInputHash = uint32_t( CcpHashFNV1( &signature.pipelineInputs[0], sizeof( Tr2ShaderPipelineInputAL ) * signature.pipelineInputs.size() ) );
		}

		return S_OK;
	}

	Tr2ShaderAL::~Tr2ShaderAL()
	{
		ReleaseShader();
	}

	void Tr2ShaderAL::Destroy()
	{
		if( IsValid() )
		{
			ReleaseShader();
		}
	}

	bool Tr2ShaderAL::IsValid() const
	{
		return m_type != INVALID_SHADER && m_shader.vertexShader != nullptr;
	}

	ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
	{
		if( m_type == INVALID_SHADER )
		{
			bytecode = Tr2ShaderBytecodeAL();
			return E_FAIL;
		}

		bytecode.bytecode = m_bytecode.empty() ? nullptr : m_bytecode.get();
		bytecode.size = m_bytecode.size();
		return S_OK;
	}

	const Tr2ShaderSignatureAL& Tr2ShaderAL::GetSignature() const
	{
		return m_signature;
	}

	void Tr2ShaderAL::ReleaseShader()
	{
		if( m_shader.vertexShader )
		{
			switch( m_type )
			{
			case VERTEX_SHADER:
				m_shader.vertexShader->Release();
				break;
			case PIXEL_SHADER:
				m_shader.pixelShader->Release();
				break;
			case COMPUTE_SHADER:
				m_shader.computeShader->Release();
				break;
			case GEOMETRY_SHADER:
				m_shader.geometryShader->Release();
				break;
			case HULL_SHADER:
				m_shader.hullShader->Release();
				break;
			case DOMAIN_SHADER:
				m_shader.domainShader->Release();
				break;
			}
			m_shader.vertexShader = nullptr;
		}
		m_type = INVALID_SHADER;
	}

	void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderAL";
		description["shader"] = std::to_string( long long( GetType() ) );
		description["size"] = std::to_string( long long( m_bytecode.size() ) );
		description["name"] = m_name;
	}

	ALResult Tr2ShaderAL::SetName( const char* name )
	{
		m_name = name;
		SetDebugName( m_shader.vertexShader, name );
		return S_OK;
	}
}
#endif
