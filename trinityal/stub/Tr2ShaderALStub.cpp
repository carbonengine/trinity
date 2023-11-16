#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_STUB

#include "Tr2ShaderALStub.h"
#include "ALLog.h"
#include "Tr2HalHelperStructures.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
	Tr2ShaderAL::Tr2ShaderAL()
		:m_type( INVALID_SHADER )
	{
	}

	ALResult Tr2ShaderAL::Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderSignatureAL&,
		const char* shaderPath,
		Tr2PrimaryRenderContextAL & )
	{
		m_bytecode.resize( "Tr2ShaderALStub::m_bytecode", bytecode.size );
		if( m_bytecode.empty() )
		{
			return E_OUTOFMEMORY;
		}
		memcpy( m_bytecode.get(), bytecode.bytecode, bytecode.size );
		m_type = type;

		return S_OK;
	}

	Tr2ShaderAL::~Tr2ShaderAL()
	{
		Destroy();
	}

	void Tr2ShaderAL::Destroy()
	{
		m_type = INVALID_SHADER;
		m_bytecode.clear();
	}

	bool Tr2ShaderAL::IsValid() const
	{
		return m_type != INVALID_SHADER && !m_bytecode.empty();
	}

	Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
	{
		return m_type;
	}

	ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
	{
		if( m_bytecode.empty() )
		{
			bytecode = Tr2ShaderBytecodeAL();
			return E_INVALIDCALL;
		}
		bytecode.bytecode = m_bytecode.get();
		bytecode.size = m_bytecode.size();
		return S_OK;
	}

	const Tr2ShaderSignatureAL& Tr2ShaderAL::GetSignature() const
	{
		return m_signature;
	}

	void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
	{
		m_type = type;
	}

	void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& ) const
	{
	}

	ALResult Tr2ShaderAL::SetName( const char* )
	{
		return S_OK;
	}
}
#endif
