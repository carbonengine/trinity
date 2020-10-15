#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2ShaderALDx9.h"
#include "ALLog.h"
#include "Tr2RenderContextDx9.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
	Tr2ShaderAL::Tr2ShaderAL()
		: m_type( INVALID_SHADER )
	{
		m_vertexShader = nullptr;
	}

	ALResult Tr2ShaderAL::Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderBytecodeAL&,
		const Tr2ShaderSignatureAL& signature,
		Tr2PrimaryRenderContextAL &renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		m_bytecode.resize( "Tr2ShaderAL::m_byteCode", bytecode.size );
		if( !m_bytecode.empty() )
		{
			memcpy( m_bytecode.get(), bytecode.bytecode, bytecode.size );
		}

		m_type = type;

		switch( type )
		{
		case VERTEX_SHADER:
			if( FAILED( renderContext.m_d3dDevice9->CreateVertexShader( (const DWORD*)bytecode.bytecode, &m_vertexShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: vertex shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		case PIXEL_SHADER:
			if( FAILED( renderContext.m_d3dDevice9->CreatePixelShader( (const DWORD*)bytecode.bytecode, &m_pixelShader ) ) )
			{
				CCP_AL_LOGERR( "Tr2ShaderAL: pixel shader creation failed" );
				m_type = INVALID_SHADER;
				return E_FAIL;
			}
			break;
		default:
			CCP_AL_LOGERR( "Tr2ShaderAL: invalid shader type" );
			m_type = INVALID_SHADER;
			return E_INVALIDARG;
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
		if( !m_vertexShader )
		{
			return;
		}

		switch( m_type )
		{
		case VERTEX_SHADER:
			m_vertexShader->Release();
			break;
		case PIXEL_SHADER:
			m_pixelShader->Release();
			break;
		}
		m_vertexShader = nullptr;
		m_type = INVALID_SHADER;
	}

	bool Tr2ShaderAL::IsValid() const
	{
		return m_type != INVALID_SHADER && m_vertexShader != nullptr;
	}

	Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
	{
		return m_type;
	}

	ALResult Tr2ShaderAL::Apply( Tr2RenderContextAL& renderContext ) const
	{
		if( !renderContext.m_d3dDevice9 )
		{
			return E_FAIL;
		}
		switch( m_type )
		{
		case VERTEX_SHADER:
			return renderContext.m_d3dDevice9->SetVertexShader( m_vertexShader );
		case PIXEL_SHADER:
			return renderContext.m_d3dDevice9->SetPixelShader( m_pixelShader );
		}
		return E_FAIL;
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

	void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderAL";
	}
}
#endif
