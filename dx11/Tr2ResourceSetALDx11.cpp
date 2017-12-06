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
		:m_isValid( false ),
		m_empty( true )
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
		for( uint32_t stageIndex = 0; stageIndex < SHADER_TYPE_COUNT; ++stageIndex )
		{
			auto& stage = m_stages[stageIndex];
			for( uint32_t registerIndex = 0; registerIndex < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++registerIndex )
			{
				auto& desc = description.m_resources[stageIndex][registerIndex];
				if( desc.type != Tr2ResourceSetDescriptionAL::NONE && registerIndex >= MAX_RESOURCES )
				{
					return E_INVALIDARG;
				}
				switch( desc.type )
				{
				case Tr2ResourceSetDescriptionAL::BUFFER:
					stage.resources[registerIndex] = desc.buffer->m_srv;
					break;
				case Tr2ResourceSetDescriptionAL::TEXTURE:
					stage.resources[registerIndex] = desc.texture->m_view[desc.colorSpace];
					break;
				case Tr2ResourceSetDescriptionAL::NONE:
					continue;
				default:
					return E_INVALIDARG;
				}
				stage.resourceOffset = std::min( stage.resourceOffset, registerIndex );
				stage.resourceCount = std::max( stage.resourceCount, registerIndex + 1 );
			}
			for( uint32_t registerIndex = 0; registerIndex < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++registerIndex )
			{
				auto& desc = description.m_samplers[stageIndex][registerIndex];
				if( desc.assigned && registerIndex >= MAX_RESOURCES )
				{
					return E_INVALIDARG;
				}
				if( desc.assigned )
				{
					stage.samplers[registerIndex] = desc.sampler.m_sampler->m_samplerState;
					stage.samplerOffset = std::min( stage.samplerOffset, registerIndex );
					stage.samplerCount = std::max( stage.samplerCount, registerIndex + 1 );
				}
			}
		}

		m_empty = true;

		for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
		{
			if( it->resourceCount > it->resourceOffset )
			{
				it->resourceHash = 0;
				for( uint32_t i = it->resourceOffset; i != it->resourceCount; ++i )
				{
					it->resourceHash ^= uint32_t( reinterpret_cast<uintptr_t>( it->resources[i].p ) & 0xffffffff ) << i;
				}
				it->resourceCount -= it->resourceOffset;
				m_empty = false;
			}
			else
			{
				it->resourceCount = 0;
				it->resourceHash = 0;
			}
			if( it->samplerCount > it->samplerOffset )
			{
				it->samplerHash = 0;
				for( uint32_t i = it->samplerOffset; i != it->samplerCount; ++i )
				{
					it->samplerHash ^= uint32_t( reinterpret_cast<uintptr_t>( it->samplers[i].p ) & 0xffffffff ) << i;
				}
				it->samplerCount -= it->samplerOffset;
				m_empty = false;
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
		m_empty = true;
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}


	Tr2ResourceSetAL::StageInput::StageInput()
		:resourceCount( 0 ),
		resourceOffset( 0 ),
		samplerCount( 0 ),
		samplerOffset( 0 ),
		resourceHash( 0 ),
		samplerHash( 0 )
	{
	}

	void Tr2ResourceSetAL::StageInput::Destroy()
	{
		std::fill_n( resources, MAX_RESOURCES, nullptr );
		std::fill_n( samplers, MAX_RESOURCES, nullptr );
		resourceCount = 0;
		samplerCount = 0;
		resourceHash = 0;
		samplerHash = 0;
	}

}

#endif