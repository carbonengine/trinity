#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX9

#include "Tr2ResourceSetALDx9.h"
#include "../include/Tr2TextureAL.h"
#include "Tr2SamplerStateALDx9.h"
#include "Tr2TextureALDx9.h"
#include "../include/Tr2ShaderProgramAL.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_isValid( false ),
		m_samplerHash( 0 )
	{
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& /*renderContext*/ )
	{
		Destroy();
		ON_BLOCK_EXIT_WITH_UNUSED( [&] { if( !IsValid() ) Destroy(); } );

		if( program.GetRegisterMap() != description.m_registerMap )
		{
			return E_INVALIDARG;
		}

		for( uint32_t stage = VERTEX_SHADER; stage <= PIXEL_SHADER; ++stage )
		{
			const uint32_t maxResources = stage == PIXEL_SHADER ? 16 : 4;
			for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_registerMap.srvs[stage][i] >= description.m_registerMap.srvCount )
				{
					continue;
				}
				auto& desc = description.m_srv[description.m_registerMap.srvs[stage][i]];
				if( desc.type == Tr2ResourceSetDescriptionAL::NONE )
				{
					continue;
				}
				if( desc.type != Tr2ResourceSetDescriptionAL::TEXTURE )
				{
					return E_INVALIDARG;
				}
				if( i >= maxResources )
				{
					return E_INVALIDARG;
				}

				Texture tex;
				tex.registerIndex = i + ( stage == VERTEX_SHADER ? D3DVERTEXTEXTURESAMPLER0 : 0 );
				tex.texture = desc.texture.m_texture->m_texture;
				m_textures.push_back( tex );

				SamplerState ss;
				ss.registerIndex = tex.registerIndex;
				ss.state = D3DSAMP_SRGBTEXTURE;
				ss.value = desc.colorSpace == COLOR_SPACE_SRGB ? TRUE : FALSE;
				m_samplerStates.push_back( ss );
			}
			for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_registerMap.samplers[stage][i] >= description.m_registerMap.samplerCount )
				{
					continue;
				}
				if( !description.m_samplers[description.m_registerMap.samplers[stage][i]].assigned )
				{
					continue;
				}
				if( i >= maxResources )
				{
					return E_INVALIDARG;
				}

				for( uint32_t j = Tr2SamplerStateAL::SAMPLER_STATE_MIN; j < Tr2SamplerStateAL::SAMPLER_STATE_COUNT; ++j )
				{
					SamplerState ss;
					ss.registerIndex = i;
					ss.state = D3DSAMPLERSTATETYPE( j );
					ss.value = description.m_samplers[description.m_registerMap.samplers[stage][i]].sampler.m_sampler->m_states[j];
					m_samplerStates.push_back( ss );
				}
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

	void Tr2ResourceSetAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ResourceSetAL";
	}

}

#endif