// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "include/Tr2RtPipelineStateAL.h"
#include "include/Tr2CapsAL.h"

#if TRINITY_PLATFORM_SUPPORTS_RAY_TRACING

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2RtPipelineStateAL )


namespace
{
std::shared_ptr<TrinityALImpl::Tr2RtPipelineStateAL> NullRtPipeline()
{
	static std::shared_ptr<TrinityALImpl::Tr2RtPipelineStateAL> nullPipeline = std::make_shared<TrinityALImpl::Tr2RtPipelineStateAL>();
	return nullPipeline;
}
}


Tr2RtPipelineStateAL::Tr2RtPipelineStateAL() :
	m_pipeline( NullRtPipeline() )
{
}

ALResult Tr2RtPipelineStateAL::CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext )
{
	m_pipeline = std::make_shared<TrinityALImpl::Tr2RtPipelineStateAL>();
	auto hr = m_pipeline->CreateRtPipelineState( desc, renderContext );
	if( FAILED( hr ) )
	{
		m_pipeline = NullRtPipeline();
	}
	return hr;
}

bool Tr2RtPipelineStateAL::IsValid() const
{
	return m_pipeline->IsValid();
}

TrinityALImpl::Tr2RtPipelineStateAL* Tr2RtPipelineStateAL::TrinityALImpl_GetObject() const
{
	return m_pipeline.get();
}

#else

Tr2RtPipelineStateAL::Tr2RtPipelineStateAL()
{
}

ALResult Tr2RtPipelineStateAL::CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext )
{
	return E_FAIL;
}

bool Tr2RtPipelineStateAL::IsValid() const
{
	return false;
}

TrinityALImpl::Tr2RtPipelineStateAL* Tr2RtPipelineStateAL::TrinityALImpl_GetObject() const
{
	return nullptr;
}

#endif

Tr2RtPipelineStateDescriptionAL::Shader::~Shader()
{
}

Tr2RtPipelineStateDescriptionAL::~Tr2RtPipelineStateDescriptionAL()
{
}

void Tr2RtPipelineStateDescriptionAL::AddShader( const wchar_t* exportName, const Tr2ShaderBytecodeAL& bytecode, const wchar_t* name, uint32_t payloadSize )
{
	ShaderName sn;
	sn.name = name;
	sn.exportName = exportName;

	Shader shader;
	shader.names.push_back( sn );
	auto code = new uint8_t[bytecode.size];
	memcpy( code, bytecode.bytecode, bytecode.size );
	shader.bytecode = Tr2ShaderBytecodeAL( code, bytecode.size );
	shader.payloadSize = payloadSize;
	shader.localSignature = NO_SIGNATURE;
	m_shaders.emplace_back( std::move( shader ) );
}

void Tr2RtPipelineStateDescriptionAL::AddShaders( size_t count, const wchar_t** exportNames, const Tr2ShaderBytecodeAL& bytecode, const wchar_t** names, uint32_t payloadSize )
{
	Shader shader;
	shader.names.reserve( count );
	for( size_t i = 0; i < count; ++i )
	{
		ShaderName sn;
		sn.name = names[i];
		sn.exportName = exportNames[i];
		shader.names.push_back( sn );
	}
	auto code = new uint8_t[bytecode.size];
	memcpy( code, bytecode.bytecode, bytecode.size );
	shader.bytecode = Tr2ShaderBytecodeAL( code, bytecode.size );
	shader.payloadSize = payloadSize;
	shader.localSignature = NO_SIGNATURE;
	m_shaders.emplace_back( std::move( shader ) );
}

void Tr2RtPipelineStateDescriptionAL::AddShaders( size_t count, const wchar_t** exportNames, const Tr2ShaderBytecodeAL& bytecode, const wchar_t** names, uint32_t payloadSize, const Tr2ShaderSignatureAL& signature )
{
	Shader shader;
	shader.names.reserve( count );
	for( size_t i = 0; i < count; ++i )
	{
		ShaderName sn;
		sn.name = names[i];
		sn.exportName = exportNames[i];
		shader.names.push_back( sn );
	}
	auto code = new uint8_t[bytecode.size];
	memcpy( code, bytecode.bytecode, bytecode.size );
	shader.bytecode = Tr2ShaderBytecodeAL( code, bytecode.size );
	shader.payloadSize = payloadSize;
	shader.localSignature = AddLocalSignature( signature );
	m_shaders.emplace_back( std::move( shader ) );
}

void Tr2RtPipelineStateDescriptionAL::AddHitGroup( const wchar_t* exportName, const wchar_t* anyHit, const wchar_t* closestHit, const wchar_t* intersection )
{
	HitGroup hitGroup = { exportName, anyHit ? anyHit : L"", closestHit ? closestHit : L"", intersection ? intersection : L"", NO_SIGNATURE };
	m_hitGroups.push_back( hitGroup );
}

void Tr2RtPipelineStateDescriptionAL::AddHitGroup( const wchar_t* exportName, const wchar_t* anyHit, const wchar_t* closestHit, const wchar_t* intersection, const Tr2ShaderSignatureAL& signature )
{
	HitGroup hitGroup = { exportName, anyHit ? anyHit : L"", closestHit ? closestHit : L"", intersection ? intersection : L"", AddLocalSignature( signature ) };
	m_hitGroups.push_back( hitGroup );
}

void Tr2RtPipelineStateDescriptionAL::AddGlobalSignature( const Tr2ShaderSignatureAL& signature )
{
	m_globalSignature = signature;
}

uint32_t Tr2RtPipelineStateDescriptionAL::AddLocalSignature( const Tr2ShaderSignatureAL& signature )
{
	if( signature.IsEmpty() )
	{
		return NO_SIGNATURE;
	}

	uint32_t signatureIndex = 0;
	for( auto it = begin( m_localSignatures ); it != end( m_localSignatures ); ++it, ++signatureIndex )
	{
		if( *it == signature )
		{
			return signatureIndex;
		}
	}
	m_localSignatures.push_back( signature );
	return uint32_t( m_localSignatures.size() - 1 );
}
