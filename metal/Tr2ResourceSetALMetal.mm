#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2ResourceSetALMetal.h"
#include "Tr2BufferALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "Tr2SamplerStateALMetal.h"
#include "Tr2TextureALMetal.h"
#include "Tr2ShaderProgramALMetal.h"
#include "ALLog.h"

static_assert(
	sizeof( TrinityALImpl::ShaderResourceMask::constantBufferMask ) * 8 >= METAL_MAX_BOUND_BUFFERS,
	"Please use a type with more bits for ShaderResourceMask::constantBufferMask." );
static_assert(
	sizeof( TrinityALImpl::ShaderResourceMask::bufferMask ) * 8 >= METAL_MAX_BOUND_BUFFERS,
	"Please use a type with more bits for ShaderResourceMask::bufferMask." );
static_assert(
	sizeof( TrinityALImpl::ShaderResourceMask::textureMask ) * 8 >= METAL_MAX_BOUND_TEXTURES,
	"Please use a type with more bits for ShaderResourceMask::textureMask." );
static_assert(
	sizeof( TrinityALImpl::ShaderResourceMask::samplerMask ) * 8 >= METAL_MAX_BOUND_SAMPLERS,
	"Please use a type with more bits for ShaderResourceMask::samplerMask." );


namespace TrinityALImpl
{
	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_isValid( false )
	{
		static_assert(
			sizeof( m_buffersMask ) * 8 >= METAL_MAX_BOUND_BUFFERS,
			"Please use a type with more bits for m_buffersMask." );

		Destroy();
	}

	Tr2ResourceSetAL::~Tr2ResourceSetAL()
	{
		Destroy();
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL &program, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() || !program.IsValid() )
		{
			return E_INVALIDARG;
		}

		using namespace Tr2RenderContextEnum;
		const ShaderType stages[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
		for( auto stage : stages )
		{
			uint32_t buffersMask = 0;
			NSUInteger texturesMin = NSUIntegerMax;
			NSUInteger texturesMax = 0;

			// Init required resource mask to that required by the shader.
			// uint32_t bufferMask  = program.m_program->m_resourceMask[stage].bufferMask;
			uint32_t textureMask = program.m_program->m_resourceMask[stage].textureMask;
			uint32_t samplerMask = program.m_program->m_resourceMask[stage].samplerMask;

			for( int i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_registerMap.srvs[stage][i] >= description.m_registerMap.srvCount )
				{
					continue;
				}
				const Tr2ResourceSetDescriptionAL::Resource& resource = description.m_srv[description.m_registerMap.srvs[stage][i]];

				switch( resource.type )
				{
				case Tr2ResourceSetDescriptionAL::BUFFER:
					if( resource.buffer.IsValid() )
					{
						CCP_ASSERT( i < METAL_SRV_BUFFER_COUNT );

						const NSUInteger bufferIndex = METAL_SRV_BUFFER_OFFSET + i;
						m_buffers[stage][bufferIndex] = resource.buffer.m_buffer->GetMetalBuffer();
						buffersMask |= (1 << bufferIndex);

						// Remove this resource from mask.
						// bufferMask &= ~(1 << bufferIndex);
					}
					break;
				case Tr2ResourceSetDescriptionAL::TEXTURE:
					if( resource.texture.IsValid() )
					{
						CCP_ASSERT( i < METAL_SRV_TEXTURE_COUNT );

						const NSUInteger texIndex = METAL_SRV_TEXTURE_OFFSET + i;

						if( resource.colorSpace == COLOR_SPACE_SRGB )
						{
							m_textures[stage][texIndex] = resource.texture.m_texture->GetSRGBViewMetalTexture();
						}
						else
						{
							m_textures[stage][texIndex] = resource.texture.m_texture->GetMetalTexture();
						}

						texturesMin = std::min<NSUInteger>( texturesMin, texIndex );
						texturesMax = std::max<NSUInteger>( texturesMax, texIndex );
						// Remove this resource from mask.
						textureMask &= ~(1 << texIndex);
					}
					break;
				case Tr2ResourceSetDescriptionAL::NONE:
					continue;
				default:
					CCP_AL_LOGWARN( "Unknown SRV resource type in resource set for register %d, stage %d", i, stage );
					return E_INVALIDARG;
				}
			}

			for( int i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_registerMap.uavs[stage][i] >= description.m_registerMap.uavCount )
				{
					continue;
				}
				const Tr2ResourceSetDescriptionAL::Resource& resource = description.m_uav[description.m_registerMap.uavs[stage][i]];

				switch (resource.type)
				{
				case Tr2ResourceSetDescriptionAL::BUFFER:
					if( resource.buffer.IsValid() )
					{
						CCP_ASSERT( i < METAL_UAV_BUFFER_COUNT );

						const NSUInteger bufferIndex = METAL_UAV_BUFFER_OFFSET + i;
						m_buffers[stage][bufferIndex] = resource.buffer.m_buffer->GetMetalBuffer();
						buffersMask |= (1 << bufferIndex);

						// Remove this resource from mask.
						// bufferMask &= ~(1 << bufferIndex);
					}
					break;
				case Tr2ResourceSetDescriptionAL::TEXTURE:
					if( resource.texture.IsValid() )
					{
						CCP_ASSERT( i < METAL_UAV_TEXTURE_COUNT );

						const NSUInteger texIndex = METAL_UAV_TEXTURE_OFFSET + i;
						m_textures[stage][texIndex] = resource.texture.m_texture->GetUAVMetalTexture( resource.mip );
						texturesMin = std::min<NSUInteger>( texturesMin, texIndex );
						texturesMax = std::max<NSUInteger>( texturesMax, texIndex );
						// Remove this resource from mask.
						textureMask &= ~(1 << texIndex);
					}
					break;
				case Tr2ResourceSetDescriptionAL::NONE:
					continue;
				default:
					CCP_AL_LOGWARN( "Unknown UAV resource type in resource set for register %d, stage %d", i, stage );
					return E_INVALIDARG;
				}
			}

			NSUInteger samplersMin = NSUIntegerMax;
			NSUInteger samplersMax = 0;

			for( int i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				if( description.m_registerMap.samplers[stage][i] >= description.m_registerMap.samplerCount )
				{
					continue;
				}
				const Tr2ResourceSetDescriptionAL::Sampler& sampler = description.m_samplers[description.m_registerMap.samplers[stage][i]];

				if( sampler.assigned )
				{
					m_samplers[stage][i] = sampler.sampler.m_sampler->GetMetalSamplerState();
					samplersMin = std::min<NSUInteger>( samplersMin, i );
					samplersMax = std::max<NSUInteger>( samplersMax, i );
					// Remove this resource from mask.
					samplerMask &= ~(1 << i);
				}
			}

			// Replace any missing resources with dummy ones.

			MetalContext* metalContext = renderContext.GetMetalContext();
			unsigned int index = 0;
			// Buffers not supported yet.
#if 0
			while( bufferMask )
			{
				if( bufferMask & 0x1 )
				{
					// Note - Buffers for vertex shaders are NOT set via a resource set, so you if you re-set them here trouble will ensue.
				}
				bufferMask >>= 1;
				++index;
			}
#endif

			index = 0;
			while( textureMask )
			{
				if( textureMask & 0x1 )
				{
					m_textures[stage][index] = metalContext->GetDummyTexture( MTLTextureType( program.m_program->m_resourceMask[stage].textureTypes[index] ) );
					texturesMin = std::min<NSUInteger>( texturesMin, index );
					texturesMax = std::max<NSUInteger>( texturesMax, index );
				}
				textureMask >>= 1;
				++index;
			}

			index = 0;
			while( samplerMask )
			{
				if( samplerMask & 0x1 )
				{
					m_samplers[stage][index] = metalContext->GetDummySampler();
					samplersMin = std::min<NSUInteger>( samplersMin, index );
					samplersMax = std::max<NSUInteger>( samplersMax, index );
				}
				samplerMask >>= 1;
				++index;
			}

			m_buffersMask[stage] = buffersMask;
			m_texturesRange[stage] = ( texturesMin != NSUIntegerMax ) ? NSMakeRange( texturesMin, texturesMax - texturesMin + 1 ) : NSMakeRange( 0, 0 );
			m_samplersRange[stage] = ( samplersMin != NSUIntegerMax ) ? NSMakeRange( samplersMin, samplersMax - samplersMin + 1 ) : NSMakeRange( 0, 0 );
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
		m_isValid = false;

		using namespace Tr2RenderContextEnum;
		const ShaderType stages[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
		for( auto stage : stages )
		{
			for( int i = 0; i < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++i )
			{
				m_buffers[stage][i] = nil;
				m_textures[stage][i] = nil;
				m_samplers[stage][i] = nil;
			}

			m_buffersMask[stage] = 0;
			m_texturesRange[stage] = NSMakeRange( 0, 0 );
			m_samplersRange[stage] = NSMakeRange( 0, 0 );
		}
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
