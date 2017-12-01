#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2ResourceSetALDx11.h"
#include "../include/Tr2GpuBufferAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2SamplerStateAL.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_isValid( false )
	{
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();
		ON_BLOCK_EXIT( [&] { if( !IsValid() ) Destroy(); } );

		for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
		{
			it->resourceOffset = MAX_RESOURCES;
			it->samplerOffset = MAX_RESOURCES;
		}
		for( auto it = std::begin( description.m_resources ); it != std::end( description.m_resources ); ++it )
		{
			if( it->first.stage < SHADER_TYPE_FIRST || it->first.stage >= SHADER_TYPE_COUNT )
			{
				return E_INVALIDARG;
			}
			if( it->first.registerIndex >= MAX_RESOURCES )
			{
				return E_INVALIDARG;
			}
			auto& stage = m_stages[it->first.stage];
			switch( it->second.type )
			{
			case Tr2ResourceSetDescriptionAL::BUFFER:
				stage.resources[it->first.registerIndex] = it->second.buffer->m_srv;
				break;
			case Tr2ResourceSetDescriptionAL::TEXTURE:
				stage.resources[it->first.registerIndex] = it->second.texture->m_view[it->second.colorSpace];
				break;
			default:
				return E_INVALIDARG;
			}
			stage.resourceOffset = std::min( stage.resourceOffset, it->first.registerIndex );
			stage.resourceCount = std::max( stage.resourceCount, it->first.registerIndex + 1 );
		}

		for( auto it = std::begin( description.m_samplers ); it != std::end( description.m_samplers ); ++it )
		{
			if( it->first.stage < SHADER_TYPE_FIRST || it->first.stage >= SHADER_TYPE_COUNT )
			{
				return E_INVALIDARG;
			}
			if( it->first.registerIndex >= MAX_RESOURCES )
			{
				return E_INVALIDARG;
			}
			auto& stage = m_stages[it->first.stage];
			stage.samplers[it->first.registerIndex] = it->second.sampler->m_sampler->m_samplerState;
			stage.samplerOffset = std::min( stage.samplerOffset, it->first.registerIndex );
			stage.samplerCount = std::max( stage.samplerCount, it->first.registerIndex + 1 );
		}
		for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
		{
			if( it->resourceCount > it->resourceOffset )
			{
				it->resourceCount -= it->resourceOffset;
			}
			else
			{
				it->resourceCount = 0;
			}
			if( it->samplerCount > it->samplerOffset )
			{
				it->samplerHash = 0;
				for( uint32_t i = it->samplerOffset; i != it->samplerCount; ++i )
				{
					it->samplerHash ^= uint32_t( reinterpret_cast<uintptr_t>( it->samplers[i].p ) & 0xffffffff ) << i;
				}
				it->samplerCount -= it->samplerOffset;
			}
			else
			{
				it->samplerCount = 0;
				it->samplerHash = 0;
			}
		}
		m_isValid = true;
		return S_OK;
	};

	bool Tr2ResourceSetAL::IsValid() const
	{
		return m_isValid;
	}

	void Tr2ResourceSetAL::Destroy()
	{
		for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
		{
			it->Destroy();
		}
		m_isValid = false;
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}


	Tr2ResourceSetAL::StageInput::StageInput()
		:resourceCount( 0 ),
		resourceOffset( 0 ),
		samplerCount( 0 ),
		samplerOffset( 0 )
	{
	}

	void Tr2ResourceSetAL::StageInput::Destroy()
	{
		std::fill_n( resources, MAX_RESOURCES, nullptr );
		std::fill_n( samplers, MAX_RESOURCES, nullptr );
		resourceCount = 0;
		samplerOffset = 0;
	}

}

#endif