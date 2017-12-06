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

		for( uint32_t stage = 0; stage < SHADER_TYPE_COUNT; ++stage )
		{
			if( stage == PIXEL_SHADER )
			{
				continue;
			}
			for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_resources[stage][i].buffer )
				{
					return E_INVALIDARG;
				}
				if( description.m_samplers[stage][i].assigned )
				{
					return E_INVALIDARG;
				}
			}
		}

		for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
		{
			if( description.m_samplers[PIXEL_SHADER][i].assigned && description.m_resources[PIXEL_SHADER][i].type == Tr2ResourceSetDescriptionAL::NONE )
			{
				return E_INVALIDARG;
			}
		}

		for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
		{
			auto& resource = description.m_resources[PIXEL_SHADER][i];
			if( resource.type == Tr2ResourceSetDescriptionAL::NONE )
			{
				continue;
			}
			if( resource.type != Tr2ResourceSetDescriptionAL::TEXTURE )
			{
				return E_INVALIDARG;
			}

			auto& sampler = description.m_samplers[PIXEL_SHADER][i];
			if( !sampler.assigned )
			{
				return E_INVALIDARG;
			}

			Texture tex;
			tex.registerIndex = i;
			tex.texture = resource.texture->m_texture;
			tex.type = ConvertTextureType( resource.texture->GetType() );
			tex.srgbDecode = resource.colorSpace == COLOR_SPACE_SRGB ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT;
			tex.sampler = sampler.sampler.m_sampler->m_stateData;
			tex.hasMips = resource.texture->GetTrueMipCount() > 1;
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