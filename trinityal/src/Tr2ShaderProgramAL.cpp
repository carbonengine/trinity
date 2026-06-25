// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2ShaderProgramAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ShaderProgramAL )


namespace
{
std::shared_ptr<TrinityALImpl::Tr2ShaderProgramAL> NullProgram()
{
	static std::shared_ptr<TrinityALImpl::Tr2ShaderProgramAL> nullProgram = std::make_shared<TrinityALImpl::Tr2ShaderProgramAL>();
	return nullProgram;
}
}


Tr2ShaderProgramAL::Tr2ShaderProgramAL() :
	m_program( NullProgram() )
{
}

ALResult Tr2ShaderProgramAL::Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
{
	m_program = std::make_shared<TrinityALImpl::Tr2ShaderProgramAL>();
	auto result = m_program->Create( shaders, count, renderContext );
	if( FAILED( result ) )
	{
		m_program = NullProgram();
	}
	return result;
}

ALResult Tr2ShaderProgramAL::CreateCommandSignatures( Tr2IndirectBufferLayoutAL& bufferLayout, Tr2PrimaryRenderContextAL& renderContext )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	return m_program->CreateCommandSignatures( bufferLayout, renderContext );
#else
	(void)bufferLayout;
	(void)renderContext;
	return E_FAIL;
#endif
}

bool Tr2ShaderProgramAL::IsValid() const
{
	return m_program->IsValid();
}

Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
{
	return m_program->GetMemoryClass();
}

bool Tr2ShaderProgramAL::operator==( const Tr2ShaderProgramAL& other ) const
{
	return m_program == other.m_program;
}

const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
{
	return m_program->GetRegisterMap();
}

ALResult Tr2ShaderProgramAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_program->SetName( name );
}

TrinityALImpl::Tr2ShaderProgramAL* Tr2ShaderProgramAL::TrinityALImpl_GetObject() const
{
	return m_program.get();
}
