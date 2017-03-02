#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2ShaderALDx11.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ShaderAL::Tr2ShaderAL()
	: m_type( INVALID_SHADER )	
{
	m_shader.vertexShader = nullptr;
	m_patchedShader.vertexShader = nullptr;
}

ALResult Tr2ShaderAL::Create(  
	Tr2PrimaryRenderContextAL &renderContext, 
	Tr2RenderContextEnum::ShaderType type, 
	const void* bytecode, 
	uint32_t bytecodeSize, 
	const void* patchedBytecode, 
	uint32_t patchedBytecodeSize, 
	const Tr2ShaderInputDefinition& inputDefinition )
{
	ReleaseShader();

	if( renderContext.m_d3dDevice11 == nullptr )
	{
		return E_FAIL;
	}

	m_type = type;

	m_bytecode.resize( "Tr2ShaderAL::m_byteCode", bytecodeSize );
	if( !m_bytecode.empty() )
	{
		memcpy( m_bytecode.get(), bytecode, bytecodeSize );
	}

	m_inputDefinition = inputDefinition;

	m_shader.vertexShader = nullptr;
	m_patchedShader.vertexShader = nullptr;

	switch( type )
	{
	case VERTEX_SHADER:
		if( FAILED( renderContext.m_d3dDevice11->CreateVertexShader( bytecode, bytecodeSize, nullptr, &m_shader.vertexShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: vertex shader creation failed" );
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		if( patchedBytecodeSize && FAILED( renderContext.m_d3dDevice11->CreateVertexShader( patchedBytecode, patchedBytecodeSize, nullptr, &m_patchedShader.vertexShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: vertex shader creation failed" );
			ReleaseShader();
			return E_FAIL;
		}
		break;
	case PIXEL_SHADER:
		if( FAILED( renderContext.m_d3dDevice11->CreatePixelShader( bytecode, bytecodeSize, nullptr, &m_shader.pixelShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: pixel shader creation failed" );
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		if( patchedBytecodeSize && FAILED( renderContext.m_d3dDevice11->CreatePixelShader( patchedBytecode, patchedBytecodeSize, nullptr, &m_patchedShader.pixelShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: pixel shader creation failed" );
			ReleaseShader();
			return E_FAIL;
		}
		break;
	case COMPUTE_SHADER:
		if( FAILED( renderContext.m_d3dDevice11->CreateComputeShader( bytecode, bytecodeSize, nullptr, &m_shader.computeShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: compute shader creation failed" );
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		if( patchedBytecodeSize && FAILED( renderContext.m_d3dDevice11->CreateComputeShader( patchedBytecode, patchedBytecodeSize, nullptr, &m_patchedShader.computeShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: compute shader creation failed" );
			ReleaseShader();
			return E_FAIL;
		}
		break;
	case GEOMETRY_SHADER:
		if( FAILED( renderContext.m_d3dDevice11->CreateGeometryShader( bytecode, bytecodeSize, nullptr, &m_shader.geometryShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: geometry shader creation failed" );
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		if( patchedBytecodeSize && FAILED( renderContext.m_d3dDevice11->CreateGeometryShader( patchedBytecode, patchedBytecodeSize, nullptr, &m_patchedShader.geometryShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: geometry shader creation failed" );
			ReleaseShader();
			return E_FAIL;
		}
		break;
	case HULL_SHADER:
		if( FAILED( renderContext.m_d3dDevice11->CreateHullShader( bytecode, bytecodeSize, nullptr, &m_shader.hullShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: hull shader creation failed" );
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		if( patchedBytecodeSize && FAILED( renderContext.m_d3dDevice11->CreateHullShader( patchedBytecode, patchedBytecodeSize, nullptr, &m_patchedShader.hullShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: hull shader creation failed" );
			ReleaseShader();
			return E_FAIL;
		}
		break;
	case DOMAIN_SHADER:
		if( FAILED( renderContext.m_d3dDevice11->CreateDomainShader( bytecode, bytecodeSize, nullptr, &m_shader.domainShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: domain shader creation failed" );
			m_type = INVALID_SHADER;
			return E_FAIL;
		}
		if( patchedBytecodeSize && FAILED( renderContext.m_d3dDevice11->CreateDomainShader( patchedBytecode, patchedBytecodeSize, nullptr, &m_patchedShader.domainShader ) ) )
		{
			CCP_AL_LOGERR( "Tr2ShaderAL: domain shader creation failed" );
			ReleaseShader();
			return E_FAIL;
		}
		break;
	default:
		CCP_AL_LOGERR( "Tr2ShaderAL: invalid shader type" );
		m_type = INVALID_SHADER;
		return E_INVALIDARG;
	}
	ChangeObjectId();
	return S_OK;
}

Tr2ShaderAL::~Tr2ShaderAL()
{
	ReleaseShader();
}

void Tr2ShaderAL::Destroy()
{
	ReleaseShader();
}

bool Tr2ShaderAL::operator==( const Tr2ShaderAL& shader ) const
{
	return m_type == shader.m_type && m_shader.vertexShader == shader.m_shader.vertexShader;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != INVALID_SHADER && m_shader.vertexShader != nullptr;
}

ALResult Tr2ShaderAL::Apply( Tr2RenderContextAL& renderContext ) const
{
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	switch( m_type )
	{
	case VERTEX_SHADER:
		renderContext.m_context->VSSetShader( m_shader.vertexShader, nullptr, 0 );
		break;
	case PIXEL_SHADER:
		renderContext.m_context->PSSetShader( m_shader.pixelShader, nullptr, 0 );
		break;
	case COMPUTE_SHADER:
		renderContext.m_context->CSSetShader( m_shader.computeShader, nullptr, 0 );
		break;
	case GEOMETRY_SHADER:
		renderContext.m_context->GSSetShader( m_shader.geometryShader, nullptr, 0 );
		break;
	case HULL_SHADER:
		renderContext.m_context->HSSetShader( m_shader.hullShader, nullptr, 0 );
		break;
	case DOMAIN_SHADER:
		renderContext.m_context->DSSetShader( m_shader.domainShader, nullptr, 0 );
		break;
	default:
		return E_FAIL;
	}

	return S_OK;
}

ALResult Tr2ShaderAL::ApplyPatchedShader( Tr2RenderContextAL& renderContext ) const
{
	if( m_patchedShader.vertexShader == nullptr )
	{
		return Apply( renderContext );
	}

	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	switch( m_type )
	{
	case VERTEX_SHADER:
		renderContext.m_context->VSSetShader( m_patchedShader.vertexShader, nullptr, 0 );
		break;
	case PIXEL_SHADER:
		renderContext.m_context->PSSetShader( m_patchedShader.pixelShader, nullptr, 0 );
		break;
	case COMPUTE_SHADER:
		renderContext.m_context->CSSetShader( m_patchedShader.computeShader, nullptr, 0 );
		break;
	case GEOMETRY_SHADER:
		renderContext.m_context->GSSetShader( m_patchedShader.geometryShader, nullptr, 0 );
		break;
	case HULL_SHADER:
		renderContext.m_context->HSSetShader( m_patchedShader.hullShader, nullptr, 0 );
		break;
	case DOMAIN_SHADER:
		renderContext.m_context->DSSetShader( m_patchedShader.domainShader, nullptr, 0 );
		break;
	default:
		return E_FAIL;
	}

	return S_OK;
}

ALResult Tr2ShaderAL::GetBytecode( const void*& bytecode, uint32_t& size ) const
{
	if( m_type == INVALID_SHADER )
	{
		bytecode = nullptr;
		size = 0;
		return E_FAIL;
	}

	bytecode = m_bytecode.empty() ? nullptr : m_bytecode.get();
	size = static_cast<uint32_t>( m_bytecode.size() );
	return S_OK;
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
	if( m_patchedShader.vertexShader )
	{
		switch( m_type )
		{
		case VERTEX_SHADER:
			m_patchedShader.vertexShader->Release();
			break;
		case PIXEL_SHADER:
			m_patchedShader.pixelShader->Release();
			break;
		case COMPUTE_SHADER:
			m_patchedShader.computeShader->Release();
			break;
		case GEOMETRY_SHADER:
			m_patchedShader.geometryShader->Release();
			break;
		case HULL_SHADER:
			m_patchedShader.hullShader->Release();
			break;
		case DOMAIN_SHADER:
			m_patchedShader.domainShader->Release();
			break;
		}
		m_patchedShader.vertexShader = nullptr;
	}
	m_type = INVALID_SHADER;
}

void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
{
	CCP_ASSERT( !IsValid() );
	m_type = type;
}

#endif
