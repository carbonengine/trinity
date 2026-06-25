// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "include/Tr2ShaderAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ShaderAL )

namespace
{
std::shared_ptr<TrinityALImpl::Tr2ShaderAL> NullShader()
{
	static std::shared_ptr<TrinityALImpl::Tr2ShaderAL> nullShader = std::make_shared<TrinityALImpl::Tr2ShaderAL>();
	return nullShader;
}
}


Tr2ShaderBytecodeAL::Tr2ShaderBytecodeAL() :
	bytecode( nullptr ),
	size( 0 )
{
}

Tr2ShaderBytecodeAL::Tr2ShaderBytecodeAL( const void* bytecode_, size_t size_ ) :
	bytecode( bytecode_ ),
	size( size_ )
{
}

Tr2ShaderBytecodeAL::~Tr2ShaderBytecodeAL()
{
	bytecode = nullptr;
}

Tr2ShaderPipelineInputAL::Tr2ShaderPipelineInputAL()
{
}

Tr2ShaderPipelineInputAL::Tr2ShaderPipelineInputAL( Tr2VertexDefinition::UsageCode usage_, uint32_t usageIndex_, uint32_t registerIndex_, Type type_, uint32_t dimension_, uint32_t usedMask_ ) :
	usage( usage_ ),
	usageIndex( usageIndex_ ),
	registerIndex( registerIndex_ ),
	usedMask( usedMask_ ),
	type( type_ ),
	dimension( dimension_ )
{
}

bool Tr2ShaderPipelineInputAL::operator==( const Tr2ShaderPipelineInputAL& other ) const
{
	return usage == other.usage && usageIndex == other.usageIndex && registerIndex == other.registerIndex && usedMask == other.usedMask;
}


Tr2ShaderRegisterAL::Tr2ShaderRegisterAL() :
	registerType( CONSTANT_BUFFER ),
	registerIndex( 0 ),
	dynamic( true )
{
}

Tr2ShaderRegisterAL::Tr2ShaderRegisterAL( RegisterType registerType_, uint32_t registerIndex_, uint32_t registerSpace_, uint32_t arrayCount_, bool dynamic_ ) :
	registerType( registerType_ ),
	registerIndex( registerIndex_ ),
	registerSpace( registerSpace_ ),
	arrayCount( arrayCount_ ),
	dynamic( dynamic_ )
{
}

bool Tr2ShaderRegisterAL::IsSrv() const
{
	return ( registerType & SRV_REGISTER_FLAG ) != 0;
}

bool Tr2ShaderRegisterAL::IsUav() const
{
	return ( registerType & UAV_REGISTER_FLAG ) != 0;
}

bool Tr2ShaderRegisterAL::operator==( const Tr2ShaderRegisterAL& other ) const
{
	return registerType == other.registerType && registerIndex == other.registerIndex;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( const Tr2ShaderPipelineInputAL& pipelineInput )
{
	pipelineInputs.push_back( pipelineInput );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( Tr2VertexDefinition::UsageCode usage, uint32_t usageIndex, uint32_t registerIndex, Tr2ShaderPipelineInputAL::Type type, uint32_t dimension, uint32_t usedMask )
{
	pipelineInputs.push_back( Tr2ShaderPipelineInputAL( usage, usageIndex, registerIndex, type, dimension, usedMask ) );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( const Tr2ShaderRegisterAL& registerDesc )
{
	registers.push_back( registerDesc );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( Tr2ShaderRegisterAL::RegisterType registerType, uint32_t registerIndex, uint32_t registerSpace, uint32_t arrayCount )
{
	registers.push_back( Tr2ShaderRegisterAL( registerType, registerIndex, registerSpace, arrayCount ) );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( const Tr2ShaderThreadGroupSizeAL& size )
{
	threadGroupSize = size;
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( const Tr2SamplerDescription& sampler, uint32_t registerIndex, uint32_t registerSpace )
{
	samplers.push_back( { sampler, registerIndex, registerSpace } );
	return *this;
}

bool Tr2ShaderSignatureAL::IsEmpty() const
{
	return registers.empty() && pipelineInputs.empty();
}

bool Tr2ShaderSignatureAL::operator==( const Tr2ShaderSignatureAL& other ) const
{
	return registers == other.registers && pipelineInputs == other.pipelineInputs;
}

Tr2ShaderAL::Tr2ShaderAL() :
	m_shader( NullShader() )
{
}

Tr2ShaderAL::Tr2ShaderAL( std::shared_ptr<TrinityALImpl::Tr2ShaderAL> shader ) :
	m_shader( shader )
{
}

ALResult Tr2ShaderAL::Create(
	Tr2RenderContextEnum::ShaderType type,
	const Tr2ShaderBytecodeAL& bytecode,
	const Tr2ShaderSignatureAL& signature,
	const char* shaderPath,
	Tr2PrimaryRenderContextAL& renderContext )
{
	m_shader = std::make_shared<TrinityALImpl::Tr2ShaderAL>();
	auto result = m_shader->Create( type, bytecode, signature, shaderPath, renderContext );
	if( FAILED( result ) )
	{
		m_shader = NullShader();
	}
	return result;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_shader->IsValid();
}

Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
{
	return m_shader->GetType();
}

ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
{
	return m_shader->GetBytecode( bytecode );
}

const Tr2ShaderSignatureAL& Tr2ShaderAL::GetSignature() const
{
	return m_shader->GetSignature();
}

bool Tr2ShaderAL::operator==( const Tr2ShaderAL& other ) const
{
	return m_shader == other.m_shader;
}

bool Tr2ShaderAL::operator!=( const Tr2ShaderAL& other ) const
{
	return m_shader != other.m_shader;
}

ALResult Tr2ShaderAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_shader->SetName( name );
}

TrinityALImpl::Tr2ShaderAL* Tr2ShaderAL::TrinityALImpl_GetObject() const
{
	return m_shader.get();
}
