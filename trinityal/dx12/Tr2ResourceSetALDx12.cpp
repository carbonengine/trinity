// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ResourceSetALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2ShaderProgramALDx12.h"
#include "Tr2BufferALDx12.h"
#include "Tr2TextureALDx12.h"
#include "Tr2SamplerStateALDx12.h"
#include "Tr2RtPipelineStateALDx12.h"
#include "Utilities.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

namespace
{
D3D12_CPU_DESCRIPTOR_HANDLE operator+( const D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset )
{
	D3D12_CPU_DESCRIPTOR_HANDLE result = { handle.ptr + offset };
	return result;
}
}

namespace TrinityALImpl
{
Tr2ResourceSetAL::Tr2ResourceSetAL() :
	m_owner( nullptr ),
	m_samplerCount( 0 ),
	m_resourceCount( 0 ),
	m_srvMask( 0 ),
	m_uavMask( 0 )
{
}

Tr2ResourceSetAL::~Tr2ResourceSetAL()
{
	Destroy();
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( !program.IsValid() )
	{
		return E_INVALIDARG;
	}

	return Create( description, program.m_program->m_rootSignature, renderContext, program.m_program->m_name.c_str() );
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2RtPipelineStateAL& pipeline, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( !pipeline.IsValid() )
	{
		return E_INVALIDARG;
	}

	return Create( description, pipeline.TrinityALImpl_GetObject()->GetGlobalRootSignature(), renderContext, "RtPipeline" );
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const Tr2RootSignatureAL& rootSignature, Tr2PrimaryRenderContextAL& renderContext, const char* shaderName )
{
	if( rootSignature.m_registerMap != description.m_registerMap )
	{
		return E_INVALIDARG;
	}

	std::vector<ID3D12Resource*> transitioned;

	auto AddTransition = [&]( ID3D12Resource* res, D3D12_RESOURCE_STATES defaultState, D3D12_RESOURCE_STATES expectedState ) {
		// TODO: verify state
		if( ( defaultState & expectedState ) == 0 && defaultState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE )
		{
			auto found = std::find( begin( transitioned ), end( transitioned ), res );
			if( found == transitioned.end() )
			{
				m_inTransitions.push_back( Transition( res, defaultState, expectedState ) );
				m_outTransitions.push_back( Transition( res, expectedState, defaultState ) );
				transitioned.push_back( res );
			}
		}
		m_usedResources.push_back( res );
	};

	for( auto it = begin( rootSignature.m_srvRegisters ); it != end( rootSignature.m_srvRegisters ); ++it )
	{
		auto& reg = *it;
		auto& resource = description.m_srv[description.m_registerMap.srvs[reg.stage][reg.index]];
		auto stateFlag = reg.stage == PIXEL_SHADER ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		m_resourceCount = std::max( reg.parameter + 1, m_resourceCount );
		m_srvMask |= 1 << reg.parameter;
		switch( resource.type )
		{
		case Tr2ResourceSetDescriptionAL::Resource::TEXTURE:
			if( resource.texture.IsValid() && it->registerType >= Tr2ShaderRegisterAL::SRV_TEXTURE1D )
			{
				m_srv[reg.parameter] = resource.texture.m_texture->m_view[resource.colorSpace];
			}
			else
			{
				m_srv[reg.parameter] = nullptr;
			}
			if( !m_srv[reg.parameter] )
			{
				m_srv[reg.parameter] = renderContext.GetNullSrvDx12( it->registerType );
			}
			else
			{
				AddTransition( resource.texture.m_texture->GetResourceDx12(), resource.texture.m_texture->m_defaultState, stateFlag );
			}
			break;
		case Tr2ResourceSetDescriptionAL::Resource::BUFFER:
			if( resource.buffer.IsValid() && it->registerType <= Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER )
			{
				m_srv[reg.parameter] = resource.buffer.m_buffer->m_srv;
			}
			else
			{
				m_srv[reg.parameter] = nullptr;
			}
			if( !m_srv[reg.parameter] )
			{
				m_srv[reg.parameter] = renderContext.GetNullSrvDx12( it->registerType );
			}
			else
			{
				AddTransition( resource.buffer.m_buffer->GetGpuResource(), resource.buffer.m_buffer->m_defaultState, stateFlag );
			}
			break;
		case Tr2ResourceSetDescriptionAL::Resource::HEAP_VIEW:
			m_srv[reg.parameter] = renderContext.GetSrvHeapView();
			break;
		default:
			m_srv[reg.parameter] = renderContext.GetNullSrvDx12( it->registerType );
			break;
		}
	}

	for( auto it = begin( rootSignature.m_uavRegisters ); it != end( rootSignature.m_uavRegisters ); ++it )
	{
		auto& reg = *it;
		auto& resource = description.m_uav[description.m_registerMap.uavs[reg.stage][reg.index]];
		m_resourceCount = std::max( reg.parameter + 1, m_resourceCount );
		m_uavMask |= 1 << reg.parameter;
		switch( resource.type )
		{
		case Tr2ResourceSetDescriptionAL::Resource::TEXTURE:
			if( resource.texture.IsValid() )
			{
				if( resource.texture.IsValid() && it->registerType >= Tr2ShaderRegisterAL::UAV_TEXTURE1D )
				{
					m_uav[reg.parameter] = resource.texture.m_texture->m_uav[resource.mip];
				}
				else
				{
					m_uav[reg.parameter] = nullptr;
				}
				if( !m_uav[reg.parameter] )
				{
					m_uav[reg.parameter] = renderContext.GetNullUavDx12( it->registerType );
				}
				else
				{
					AddTransition( resource.texture.m_texture->GetResourceDx12(), resource.texture.m_texture->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
				}
			}
			break;
		case Tr2ResourceSetDescriptionAL::Resource::BUFFER:
			if( resource.buffer.IsValid() )
			{
				if( resource.buffer.IsValid() && it->registerType <= Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER )
				{
					m_uav[reg.parameter] = resource.buffer.m_buffer->m_uav;
				}
				else
				{
					m_uav[reg.parameter] = nullptr;
				}
				if( !m_uav[reg.parameter] )
				{
					m_uav[reg.parameter] = renderContext.GetNullUavDx12( it->registerType );
				}
				else
				{
					AddTransition( resource.buffer.m_buffer->GetGpuResource(), resource.buffer.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
				}
			}
			break;
		case Tr2ResourceSetDescriptionAL::Resource::HEAP_VIEW:
			m_uav[reg.parameter] = renderContext.GetUavHeapView();
			break;
		default:
			CCP_AL_LOGWARN( "Missing UAV resource in resource set for register %u, stage %u, shader '%s'", reg.index, reg.stage, shaderName );
			m_uav[reg.parameter] = renderContext.GetNullUavDx12( it->registerType );
			break;
		}
	}

	for( auto it = begin( rootSignature.m_samplerRegisters ); it != end( rootSignature.m_samplerRegisters ); ++it )
	{
		auto& reg = *it;
		auto& sampler = description.m_samplers[description.m_registerMap.samplers[reg.stage][reg.index]];
		switch( sampler.type )
		{
		case Tr2ResourceSetDescriptionAL::Sampler::SAMPLER:
			m_sampler[reg.parameter] = sampler.sampler.m_sampler->m_samplerState;
			break;
		case Tr2ResourceSetDescriptionAL::Sampler::HEAP_VIEW:
			m_sampler[reg.parameter] = renderContext.GetSamplerHeapView();
			break;
		default:
			m_sampler[reg.parameter] = renderContext.GetNullSamplerDx12();
			break;
		}
		m_samplerCount = std::max( reg.parameter + 1, m_samplerCount );
	}

	m_owner = &renderContext;

	return S_OK;
}

void Tr2ResourceSetAL::Destroy()
{
	for( uint32_t idx = 0; idx < m_resourceCount; ++idx )
	{
		m_srv[idx] = nullptr;
		m_uav[idx] = nullptr;
	}
	m_resourceCount = 0;
	m_srvMask = 0;
	m_uavMask = 0;
	for( uint32_t idx = 0; idx < m_samplerCount; ++idx )
	{
		m_sampler[idx] = nullptr;
	}
	m_samplerCount = 0;

	m_owner = nullptr;
	m_inTransitions.clear();
	m_outTransitions.clear();
	m_usedResources.clear();
}

bool Tr2ResourceSetAL::IsValid() const
{
	return m_owner != nullptr;
}

Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2ResourceSetAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2ResourceSetAL";
	description["name"] = m_name;
}

ALResult Tr2ResourceSetAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}
}

#endif