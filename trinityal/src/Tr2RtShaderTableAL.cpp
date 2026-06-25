// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2RtShaderTableAL.h"
#include "include/Tr2CapsAL.h"

#if TRINITY_PLATFORM_SUPPORTS_RAY_TRACING

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2RtShaderTableAL )


namespace
{
std::shared_ptr<TrinityALImpl::Tr2RtShaderTableAL> NullRtShaderTable()
{
	static std::shared_ptr<TrinityALImpl::Tr2RtShaderTableAL> nullTable = std::make_shared<TrinityALImpl::Tr2RtShaderTableAL>();
	return nullTable;
}
}


Tr2RtShaderTableAL::Tr2RtShaderTableAL() :
	m_shaderTable( NullRtShaderTable() )
{
}

ALResult Tr2RtShaderTableAL::Create( const Tr2RtShaderTableDescriptionAL& desc, const Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
	m_shaderTable = std::make_shared<TrinityALImpl::Tr2RtShaderTableAL>();
	auto hr = m_shaderTable->Create( desc, pipeline, renderContext );
	if( FAILED( hr ) )
	{
		m_shaderTable = NullRtShaderTable();
	}
	return hr;
}

bool Tr2RtShaderTableAL::IsValid() const
{
	return m_shaderTable->IsValid();
}

TrinityALImpl::Tr2RtShaderTableAL* Tr2RtShaderTableAL::TrinityALImpl_GetObject() const
{
	return m_shaderTable.get();
}

#else

Tr2RtShaderTableAL::Tr2RtShaderTableAL()
{
}

ALResult Tr2RtShaderTableAL::Create( const Tr2RtShaderTableDescriptionAL& desc, const Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
	return E_FAIL;
}

bool Tr2RtShaderTableAL::IsValid() const
{
	return false;
}

TrinityALImpl::Tr2RtShaderTableAL* Tr2RtShaderTableAL::TrinityALImpl_GetObject() const
{
	return nullptr;
}

#endif

Tr2RtLocalMaterialDescriptionAL& Tr2RtLocalMaterialDescriptionAL::SetConstants( uint32_t registerIndex, const Tr2ConstantBufferAL& buffer )
{
	m_constants[registerIndex] = &buffer;
	return *this;
}

void Tr2RtShaderTableDescriptionAL::AddRayGenShader( const wchar_t* name )
{
	ShaderRecord shaderRecord = { name };
	m_rayGenNames.push_back( shaderRecord );
}

void Tr2RtShaderTableDescriptionAL::AddRayGenShader( const wchar_t* name, const Tr2RtLocalMaterialDescriptionAL& material )
{
	ShaderRecord shaderRecord = { name, material };
	m_rayGenNames.push_back( shaderRecord );
}

void Tr2RtShaderTableDescriptionAL::AddMissShader( const wchar_t* name )
{
	ShaderRecord shaderRecord = { name };
	m_missNames.push_back( shaderRecord );
}

void Tr2RtShaderTableDescriptionAL::AddMissShader( const wchar_t* name, const Tr2RtLocalMaterialDescriptionAL& material )
{
	ShaderRecord shaderRecord = { name, material };
	m_missNames.push_back( shaderRecord );
}

void Tr2RtShaderTableDescriptionAL::AddHitGroup( const wchar_t* name )
{
	ShaderRecord shaderRecord = { name };
	m_hitGroupNames.push_back( shaderRecord );
}

void Tr2RtShaderTableDescriptionAL::AddHitGroup( const wchar_t* name, const Tr2RtLocalMaterialDescriptionAL& material )
{
	ShaderRecord shaderRecord = { name, material };
	m_hitGroupNames.push_back( shaderRecord );
}

void Tr2RtShaderTableDescriptionAL::Reserve( size_t hitGroupCount )
{
	m_hitGroupNames.reserve( hitGroupCount );
}
