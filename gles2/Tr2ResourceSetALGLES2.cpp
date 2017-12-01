#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_OPENGLES2

#include "Tr2ResourceSetALGLES2.h"
#include "../include/Tr2TextureAL.h"

using namespace Tr2RenderContextEnum;

namespace
{

GLenum ConvertTextureType( TextureType type )
{
	switch( type )
	{
	case TEX_TYPE_CUBE:
		return GL_TEXTURE_CUBE_MAP;
	case TEX_TYPE_3D:
#ifdef TRINITY_AL_MOBILE
		return GL_TEXTURE_3D_OES;
#else
		return GL_TEXTURE_3D;
#endif
	default:
		return GL_TEXTURE_2D;
	}
}

}
namespace TrinityALImpl
{
	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_isValid( false )
	{
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& )
	{
		Destroy();
		ON_BLOCK_EXIT( [&] { if( !IsValid() ) Destroy(); } );

		for( auto it = std::begin( description.m_samplers ); it != std::end( description.m_samplers ); ++it )
		{
			auto found = description.m_resources.find( it->first );
			if( found == description.m_resources.end() )
			{
				return E_INVALIDARG;
			}
		}

		for( auto it = std::begin( description.m_resources ); it != std::end( description.m_resources ); ++it )
		{
			if( it->first.stage != PIXEL_SHADER )
			{
				return E_INVALIDARG;
			}
			if( it->first.registerIndex >= 16 )
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

			auto sampler = description.m_samplers.find( it->first );
			if( sampler == description.m_samplers.end() )
			{
				return E_INVALIDARG;
			}

			Texture tex;
			tex.registerIndex = it->first.registerIndex;
			tex.texture = it->second.texture->m_texture;
			tex.type = ConvertTextureType( it->second.texture->GetType() );
			tex.srgbDecode = it->second.colorSpace == COLOR_SPACE_SRGB ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT;
			tex.sampler = sampler->second.sampler->m_sampler->m_stateData;
			tex.hasMips = it->second.texture->GetTrueMipCount() > 1;
			m_textures.push_back( tex );
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
		m_isValid = false;
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}
}

#endif