#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_OPENGL4

#include "Tr2ResourceSetALGL4.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2RenderContextAL.h"
#include "../include/Tr2GpuBufferAL.h"

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

		for( auto it = std::begin( description.m_samplers ); it != std::end( description.m_samplers ); ++it )
		{
			if( it->first.stage == COMPUTE_SHADER )
			{
				continue;
			}
			auto found = description.m_resources.find( it->first );
			if( found == description.m_resources.end() )
			{
				return E_INVALIDARG;
			}
		}

		for( auto it = std::begin( description.m_resources ); it != std::end( description.m_resources ); ++it )
		{
			if( it->first.stage != PIXEL_SHADER && it->first.stage != COMPUTE_SHADER )
			{
				return E_INVALIDARG;
			}
			if( it->first.registerIndex >= 16 )
			{
				return E_INVALIDARG;
			}
			if( it->first.stage == PIXEL_SHADER && it->second.type != Tr2ResourceSetDescriptionAL::TEXTURE )
			{
				return E_INVALIDARG;
			}

			if( it->first.stage == PIXEL_SHADER )
			{
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
				tex.sampler = sampler->second.sampler->m_sampler->m_sampler;
				m_textures.push_back( tex );
			}
			else
			{
				CLResource clResource;
				clResource.registerIndex = it->first.registerIndex;
				clResource.isSampler = false;

				switch( it->second.type )
				{
				case Tr2ResourceSetDescriptionAL::TEXTURE:
				{
					auto& texture = *it->second.texture;
					if( !texture.m_clObject )
					{
						switch( it->second.texture->GetType() )
						{
						case TEX_TYPE_1D:
						case TEX_TYPE_2D:
#ifdef _WIN32
							texture.m_clObject = clCreateFromGLTexture2D( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, *texture.m_texture, nullptr );
#else
							texture.m_clObject = clCreateFromGLTexture( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, *texture.m_texture, nullptr );
#endif
							break;
						case TEX_TYPE_3D:
#ifdef _WIN32
							texture.m_clObject = clCreateFromGLTexture3D( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, *it->second.texture->m_texture, nullptr );
#else
							texture.m_clObject = clCreateFromGLTexture( renderContext.m_clContext, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, *texture.m_texture, nullptr );
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
					auto& buffer = *it->second.buffer;
					if( !buffer.m_clObject )
					{
						int error;
						buffer.m_clObject = clCreateFromGLBuffer( renderContext.m_clContext, CL_MEM_READ_WRITE, *buffer.m_buffer, &error );
						if( !buffer.m_clObject )
						{
							return E_FAIL;
						}
					}
					clResource.resource = buffer.m_clObject;
					clRetainMemObject( buffer.m_clObject );
					break;
				}
				default:
					return E_INVALIDARG;
				}
				m_clResources.push_back( clResource );
			}
		}
		for( auto it = std::begin( description.m_samplers ); it != std::end( description.m_samplers ); ++it )
		{
			if( it->first.stage != PIXEL_SHADER && it->first.stage != COMPUTE_SHADER )
			{
				return E_INVALIDARG;
			}
			if( it->first.registerIndex >= 16 )
			{
				return E_INVALIDARG;
			}
			if( it->first.stage == PIXEL_SHADER )
			{
				if( description.m_resources.find( it->first ) == description.m_resources.end() )
				{
					return E_INVALIDARG;
				}
			}
			else
			{
				auto& sampler = *it->second.sampler->m_sampler;
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
				clResource.registerIndex = it->first.registerIndex;
				clResource.isSampler = true;
				clResource.sampler = sampler.m_clObject;
				clRetainSampler( sampler.m_clObject );
				m_clResources.push_back( clResource );
			}
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