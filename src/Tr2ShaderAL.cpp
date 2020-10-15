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


Tr2ShaderBytecodeAL::Tr2ShaderBytecodeAL()
	:bytecode( nullptr ),
	size( 0 )
{
}

Tr2ShaderBytecodeAL::Tr2ShaderBytecodeAL( const void* bytecode_, size_t size_ )
	: bytecode( bytecode_ ),
	size( size_ )
{
}


Tr2ShaderPipelineInputAL::Tr2ShaderPipelineInputAL()
{
}

Tr2ShaderPipelineInputAL::Tr2ShaderPipelineInputAL( Tr2VertexDefinition::UsageCode usage_, uint32_t usageIndex_, uint32_t registerIndex_, uint32_t usedMask_ )
	:usage( usage_ ),
	usageIndex( usageIndex_ ),
	registerIndex( registerIndex_ ),
	usedMask( usedMask_ )
{
}


Tr2ShaderRegisterAL::Tr2ShaderRegisterAL()
	:registerType( CONSTANTS ),
	registerIndex( 0 )
{
}

Tr2ShaderRegisterAL::Tr2ShaderRegisterAL( RegisterType registerType_, uint32_t registerIndex_ )
	: registerType( registerType_ ),
	registerIndex( registerIndex_ )
{
}


Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( const Tr2ShaderPipelineInputAL& pipelineInput )
{
	pipelineInputs.push_back( pipelineInput );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( Tr2VertexDefinition::UsageCode usage, uint32_t usageIndex, uint32_t registerIndex, uint32_t usedMask )
{
	pipelineInputs.push_back( Tr2ShaderPipelineInputAL( usage, usageIndex, registerIndex, usedMask ) );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( const Tr2ShaderRegisterAL& registerDesc )
{
	registers.push_back( registerDesc );
	return *this;
}

Tr2ShaderSignatureAL& Tr2ShaderSignatureAL::Add( Tr2ShaderRegisterAL::RegisterType registerType, uint32_t registerIndex )
{
	registers.push_back( Tr2ShaderRegisterAL( registerType, registerIndex ) );
	return *this;
}



Tr2ShaderAL::Tr2ShaderAL()
	:m_shader( NullShader() )
{
}

Tr2ShaderAL::Tr2ShaderAL( std::shared_ptr<TrinityALImpl::Tr2ShaderAL> shader )
	:m_shader( shader )
{
}

ALResult Tr2ShaderAL::Create(
	Tr2RenderContextEnum::ShaderType type,
	const Tr2ShaderBytecodeAL& bytecode,
	const Tr2ShaderBytecodeAL& patchedBytecode,
	const Tr2ShaderSignatureAL& signature,
	Tr2PrimaryRenderContextAL &renderContext )
{
	m_shader = std::make_shared<TrinityALImpl::Tr2ShaderAL>();
	auto result = m_shader->Create( type, bytecode, patchedBytecode, signature, renderContext );
	if( FAILED( result ) )
	{
		m_shader = NullShader();
	}
	return result;
}

ALResult Tr2ShaderAL::Create(
	Tr2RenderContextEnum::ShaderType type,
	const Tr2ShaderBytecodeAL& bytecode,
	const Tr2ShaderSignatureAL& signature,
	Tr2PrimaryRenderContextAL &renderContext )
{
	return Create( type, bytecode, Tr2ShaderBytecodeAL(), signature, renderContext );
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
