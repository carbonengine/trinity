#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX9

#include "Tr2ResourceSetALDx9.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2SamplerStateAL.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_isValid( false ),
		m_samplerHash( 0 )
	{
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();
		ON_BLOCK_EXIT( [&] { if( !IsValid() ) Destroy(); } );

		for( auto it = std::begin( description.m_resources ); it != std::end( description.m_resources ); ++it )
		{
			if( it->first.stage != VERTEX_SHADER && it->first.stage != PIXEL_SHADER )
			{
				return E_INVALIDARG;
			}
			const uint32_t maxResources = it->first.stage == PIXEL_SHADER ? 16 : 4;
			if( it->first.registerIndex >= maxResources )
			{
				return E_INVALIDARG;
			}
			if( it->second.type != Tr2ResourceSetDescriptionAL::TEXTURE )
			{
				return E_INVALIDARG;
			}
			if( description.m_samplers.find( it->first ) == description.m_samplers.end() )
			{
				return E_INVALIDARG;
			}
			Texture tex;
			tex.registerIndex = it->first.registerIndex + ( it->first.stage == VERTEX_SHADER ? D3DVERTEXTEXTURESAMPLER0 : 0 );
			tex.texture = it->second.texture->m_texture;
			m_textures.push_back( tex );
			
			SamplerState ss;
			ss.registerIndex = tex.registerIndex;
			ss.state = D3DSAMP_SRGBTEXTURE;
			ss.value = it->second.colorSpace == COLOR_SPACE_SRGB ? TRUE : FALSE;
			m_samplerStates.push_back( ss );
		}

		for( auto it = std::begin( description.m_samplers ); it != std::end( description.m_samplers ); ++it )
		{
			if( it->first.stage != VERTEX_SHADER && it->first.stage != PIXEL_SHADER )
			{
				return E_INVALIDARG;
			}
			const uint32_t maxResources = it->first.stage == PIXEL_SHADER ? 16 : 4;
			if( it->first.registerIndex >= maxResources )
			{
				return E_INVALIDARG;
			}
			if( description.m_resources.find( it->first ) == description.m_resources.end() )
			{
				return E_INVALIDARG;
			}

			auto registerIndex = it->first.registerIndex + ( it->first.stage == VERTEX_SHADER ? D3DVERTEXTEXTURESAMPLER0 : 0 );

			for( uint32_t i = Tr2SamplerStateAL::SAMPLER_STATE_MIN; i < Tr2SamplerStateAL::SAMPLER_STATE_COUNT; ++i )
			{
				SamplerState ss;
				ss.registerIndex = registerIndex;
				ss.state = D3DSAMPLERSTATETYPE( i );
				ss.value = it->second.sampler->m_sampler->m_states[i];
				m_samplerStates.push_back( ss );
			}
		}

		if( m_samplerStates.empty() )
		{
			m_samplerHash = 0;
		}
		else
		{
			m_samplerHash = CcpHashFNV1( &m_samplerStates[0], sizeof( m_samplerStates[0] ) * m_samplerStates.size() );
		}
		m_isValid = true;
		return S_OK;
	}

	bool Tr2ResourceSetAL::IsValid() const
	{
		return m_isValid;
	}

	void Tr2ResourceSetAL::Destroy()
	{
		m_textures.clear();
		m_samplerStates.clear();
		m_samplerHash = 0;
		m_isValid = false;
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}

}

#endif