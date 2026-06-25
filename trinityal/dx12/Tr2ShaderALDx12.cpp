// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ShaderALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"

namespace TrinityALImpl
{

Tr2ShaderAL::Tr2ShaderAL() :
	m_type( Tr2RenderContextEnum::INVALID_SHADER )
{
}

ALResult Tr2ShaderAL::Create(
	Tr2RenderContextEnum::ShaderType type,
	const Tr2ShaderBytecodeAL& bytecode,
	const Tr2ShaderSignatureAL& signature,
	const char* shaderPath,
	Tr2PrimaryRenderContextAL& renderContext )
{
	m_bytecode.resize( "Tr2ShaderAL::m_bytecode", bytecode.size );
	if( m_bytecode.empty() )
	{
		return E_OUTOFMEMORY;
	}
	memcpy( m_bytecode.get(), bytecode.bytecode, bytecode.size );
	m_signature = signature;
	m_type = type;
	m_name = shaderPath ? shaderPath : "";

	renderContext.AddShaderBinaryToCrashTracker( bytecode, shaderPath );

	return S_OK;
}

void Tr2ShaderAL::Destroy()
{
	m_bytecode.clear();
	m_signature = Tr2ShaderSignatureAL();
	m_type = Tr2RenderContextEnum::INVALID_SHADER;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != Tr2RenderContextEnum::INVALID_SHADER;
}

Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
{
	return m_type;
}

Tr2ALMemoryType Tr2ShaderAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
{
	if( !IsValid() )
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

void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2ShaderAL";
	description["shader"] = std::to_string( int( GetType() ) );
	description["size"] = std::to_string( m_bytecode.size() );
	description["name"] = m_name;
}

ALResult Tr2ShaderAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}
}

#endif