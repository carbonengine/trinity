#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_OPENGL4

#include "Tr2ResourceSetALGL4.h"
#include "Tr2BufferALGL4.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2RenderContextAL.h"

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
			return GL_TEXTURE_3D;
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

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();
		ON_BLOCK_EXIT( [&] { if( !IsValid() ) Destroy(); } );

		for( uint32_t stage = VERTEX_SHADER; stage < SHADER_TYPE_COUNT; ++stage )
		{
			if( stage == COMPUTE_SHADER || stage == PIXEL_SHADER )
			{
				continue;
			}
			for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_resources[stage][i].type != Tr2ResourceSetDescriptionAL::NONE )
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
			auto& desc = description.m_resources[PIXEL_SHADER][i];
			if( desc.type == Tr2ResourceSetDescriptionAL::NONE )
			{
				continue;
			}
			if( desc.type != Tr2ResourceSetDescriptionAL::TEXTURE )
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
			tex.texture = desc.texture.m_texture->m_texture;
			tex.type = ConvertTextureType( desc.texture.GetDesc().GetType() );
			tex.srgbDecode = desc.colorSpace == COLOR_SPACE_SRGB ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT;
			tex.sampler = sampler.sampler.m_sampler->m_sampler;
			m_textures.push_back( tex );
		}

		for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
		{
			if( description.m_samplers[PIXEL_SHADER][i].assigned )
			{
				if( description.m_resources[PIXEL_SHADER][i].type == Tr2ResourceSetDescriptionAL::NONE )
				{
					return E_INVALIDARG;
				}
			}
		}

		for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
		{
			auto& desc = description.m_resources[COMPUTE_SHADER][i];
			if( desc.type == Tr2ResourceSetDescriptionAL::NONE )
			{
				continue;
			}

			CLResource clResource;
			clResource.registerIndex = i;
			clResource.isSampler = false;

			switch( desc.type )
			{
			case Tr2ResourceSetDescriptionAL::TEXTURE:
			{
				auto& texture = *desc.texture.m_texture;
				if( !texture.m_clObject )
				{
					switch( desc.texture.GetDesc().GetType() )
					{
					case TEX_TYPE_1D:
					case TEX_TYPE_2D:
#ifdef _WIN32
						texture.m_clObject = clCreateFromGLTexture2D( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texture.m_texture, nullptr );
#else
						texture.m_clObject = clCreateFromGLTexture( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texture.m_texture, nullptr );
#endif
						break;
					case TEX_TYPE_3D:
#ifdef _WIN32
						texture.m_clObject = clCreateFromGLTexture3D( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, texture.m_texture, nullptr );
#else
						texture.m_clObject = clCreateFromGLTexture( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, texture.m_texture, nullptr );
#endif
						break;
					default:
						return E_FAIL;
					}
					if( !texture.m_clObject )
					{
						return E_FAIL;
					}
				}
				clResource.resource = texture.m_clObject;
				clRetainMemObject( texture.m_clObject );
				break;
			}
			case Tr2ResourceSetDescriptionAL::BUFFER:
			{
				auto& buffer = desc.buffer;
				if( !buffer.m_buffer->m_clObject )
				{
					int error;
					buffer.m_buffer->m_clObject = clCreateFromGLBuffer( renderContext.m_clContext, CL_MEM_READ_WRITE, buffer.m_buffer->m_buffer, &error );
					if( !buffer.m_buffer->m_clObject )
					{
						return E_FAIL;
					}
				}
				clResource.resource = buffer.m_buffer->m_clObject;
				clRetainMemObject( buffer.m_buffer->m_clObject );
				break;
			}
			default:
				return E_INVALIDARG;
			}
			m_clResources.push_back( clResource );
		}

		for( uint32_t i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
		{
			auto& sampler = *description.m_samplers[COMPUTE_SHADER][i].sampler.m_sampler;
			if( !sampler.IsValid() )
			{
				continue;
			}
			if( !sampler.m_clObject )
			{
				sampler.m_clObject = clCreateSampler( renderContext.m_clContext, CL_TRUE,
					sampler.m_stateData.m_wrapT == GL_TEXTURE_WRAP_S ? CL_ADDRESS_REPEAT : CL_ADDRESS_CLAMP,
					sampler.m_stateData.m_magFilter == GL_NEAREST ? CL_FILTER_NEAREST : CL_FILTER_LINEAR,
					nullptr );
				if( !sampler.m_clObject )
				{
					return E_FAIL;
				}
			}
			CLResource clResource;
			clResource.registerIndex = i;
			clResource.isSampler = true;
			clResource.sampler = sampler.m_clObject;
			clRetainSampler( sampler.m_clObject );
			m_clResources.push_back( clResource );
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
		for( auto it = std::begin( m_clResources ); it != std::end( m_clResources ); ++it )
		{
			if( it->sampler )
			{
				clReleaseSampler( it->sampler );
				it->sampler = nullptr;
			}
			if( it->resource )
			{
				clReleaseMemObject( it->resource );
				it->resource = nullptr;
			}
		}
		m_clResources.clear();
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}

	Tr2ResourceSetAL::CLResource::CLResource()
		:resource( nullptr ),
		sampler( nullptr ),
		isSampler( false )
	{
	}
}


#endif