// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2ResourceBindingsMetal.h"

#include "MetalContext.h"
#include "Tr2BufferALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "Tr2SamplerStateALMetal.h"
#include "Tr2ShaderProgramALMetal.h"
#include "Tr2TextureALMetal.h"

Tr2ResourceBindings::Tr2ResourceBindings() :
	m_program( nullptr ),
	m_committed( false ),
	m_sealed( false )
{
}

void Tr2ResourceBindings::Clear() throw()
{
	if( !m_program )
	{
		return;
	}
	const Tr2RegisterMapAL& registerMap = m_program->GetRegisterMap();
	std::fill( m_srvs, m_srvs + registerMap.srvCount, Resource() );
	std::fill( m_uavs, m_uavs + registerMap.uavCount, Resource() );
	std::fill( m_samplers, m_samplers + registerMap.samplerCount, Sampler() );
}

void Tr2ResourceBindings::SetProgram( const TrinityALImpl::Tr2ShaderProgramAL* program ) throw()
{
	if( program == m_program )
	{
		return;
	}
	Clear();
	m_program = program;
	m_committed = false;
}

void Tr2ResourceBindings::BeginBatch() throw()
{
	if( !m_sealed )
	{
		return;
	}

	m_sealed = false;
	m_committed = false;

	Clear();
}

void Tr2ResourceBindings::Discard() throw()
{
	Clear();

	m_program = nullptr;
	m_committed = false;
	m_sealed = false;
}

ALResult Tr2ResourceBindings::Reset() throw()
{
	Clear();

	m_committed = false;
	m_sealed = false;
	return S_OK;
}

Tr2ResourceBindings::Resource* Tr2ResourceBindings::GetSrvSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_program )
	{
		return nullptr;
	}
	BeginBatch();
	m_committed = false;

	const Tr2RegisterMapAL& registerMap = m_program->GetRegisterMap();
	uint32_t index = registerMap.srvs[stage][registerIndex];
	return index < registerMap.srvCount ? &m_srvs[index] : nullptr;
}

Tr2ResourceBindings::Resource* Tr2ResourceBindings::GetUavSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_program )
	{
		return nullptr;
	}
	BeginBatch();
	m_committed = false;

	const Tr2RegisterMapAL& registerMap = m_program->GetRegisterMap();
	uint32_t index = registerMap.uavs[stage][registerIndex];
	return index < registerMap.uavCount ? &m_uavs[index] : nullptr;
}

Tr2ResourceBindings::Sampler* Tr2ResourceBindings::GetSamplerSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_program )
	{
		return nullptr;
	}
	BeginBatch();
	m_committed = false;

	const Tr2RegisterMapAL& registerMap = m_program->GetRegisterMap();
	uint32_t index = registerMap.samplers[stage][registerIndex];
	return index < registerMap.samplerCount ? &m_samplers[index] : nullptr;
}

ALResult Tr2ResourceBindings::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( stage, registerIndex ) )
	{
		slot->type = Resource::BUFFER;
		slot->buffer = buffer;
		slot->texture = Tr2TextureAL();
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( stage, registerIndex ) )
	{
		slot->type = Resource::TEXTURE;
		slot->texture = texture;
		slot->buffer = Tr2BufferAL();
		slot->colorSpace = colorSpace;
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( stage, registerIndex ) )
	{
		slot->type = Resource::BUFFER;
		slot->buffer = buffer;
		slot->texture = Tr2TextureAL();
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( stage, registerIndex ) )
	{
		slot->type = Resource::TEXTURE;
		slot->texture = texture;
		slot->buffer = Tr2BufferAL();
		slot->mip = mip;
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( stage, registerIndex ) )
	{
		*slot = Resource();
		slot->type = Resource::HEAP_VIEW;
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( stage, registerIndex ) )
	{
		*slot = Resource();
		slot->type = Resource::HEAP_VIEW;
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSamplerSlot( stage, registerIndex ) )
	{
		slot->type = Sampler::SAMPLER;
		slot->sampler = sampler;
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSamplerSlot( stage, registerIndex ) )
	{
		slot->type = Sampler::HEAP_VIEW;
		slot->sampler = Tr2SamplerStateAL();
	}
	return S_OK;
}

ALResult Tr2ResourceBindings::Commit( Tr2RenderContextAL& context ) throw()
{
	if( !m_program )
	{
		return E_INVALIDCALL;
	}
	if( m_committed )
	{
		return S_OK;
	}

	const Tr2RegisterMapAL& registerMap = m_program->GetRegisterMap();
	const TrinityALImpl::ShaderResourceMask* resourceMasks = m_program->GetResourceMasks();

	TrinityALImpl::MetalContext* metalContext = context.GetMetalContext();
	TrinityALImpl::MetalWorkQueue* workQueue = context.GetMetalWorkQueue();
	id<MTLBuffer> heapView = metalContext->GetHeapViewBuffer();

	const Tr2RenderContextEnum::ShaderType stages[] = { Tr2RenderContextEnum::VERTEX_SHADER, Tr2RenderContextEnum::PIXEL_SHADER, Tr2RenderContextEnum::COMPUTE_SHADER };
	for( auto stage : stages )
	{
		id<MTLBuffer> buffers[METAL_MAX_BOUND_BUFFERS] = {};
		id<MTLTexture> textures[METAL_MAX_BOUND_TEXTURES] = {};
		id<MTLSamplerState> samplers[METAL_MAX_BOUND_SAMPLERS] = {};

		uint32_t buffersMask = 0;
		uint32_t heapViewMask = 0;
		NSUInteger texturesMin = NSUIntegerMax;
		NSUInteger texturesMax = 0;
		NSUInteger samplersMin = NSUIntegerMax;
		NSUInteger samplersMax = 0;

		// Resources the shader declares but nothing bound; filled with dummies below.
		uint32_t missingTextureMask = resourceMasks[stage].textureMask;
		uint32_t missingSamplerMask = resourceMasks[stage].samplerMask;

		for( uint32_t reg = 0; reg < Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE; ++reg )
		{
			const uint32_t srvIndex = registerMap.srvs[stage][reg];
			if( srvIndex < registerMap.srvCount )
			{
				const Resource& resource = m_srvs[srvIndex];
				switch( resource.type )
				{
				case Resource::BUFFER:
					if( resource.buffer.IsValid() && reg < METAL_SRV_BUFFER_COUNT )
					{
						const NSUInteger bufferIndex = METAL_SRV_BUFFER_OFFSET + reg;
						buffers[bufferIndex] = resource.buffer.TrinityALImpl_GetObject()->GetMetalBuffer();
						buffersMask |= ( 1u << bufferIndex );
					}
					break;
				case Resource::TEXTURE:
					if( resource.texture.IsValid() && reg < METAL_SRV_TEXTURE_COUNT )
					{
						const NSUInteger texIndex = METAL_SRV_TEXTURE_OFFSET + reg;
						textures[texIndex] = ( resource.colorSpace == Tr2RenderContextEnum::COLOR_SPACE_SRGB ) ?
							resource.texture.TrinityALImpl_GetObject()->GetSRGBViewMetalTexture() :
							resource.texture.TrinityALImpl_GetObject()->GetMetalTexture();
						texturesMin = std::min<NSUInteger>( texturesMin, texIndex );
						texturesMax = std::max<NSUInteger>( texturesMax, texIndex );
						missingTextureMask &= ~( 1u << texIndex );
					}
					break;
				case Resource::HEAP_VIEW:
					if( reg < METAL_SRV_BUFFER_COUNT )
					{
						heapViewMask |= ( 1u << ( METAL_SRV_BUFFER_OFFSET + reg ) );
					}
					break;
				default:
					break;
				}
			}

			const uint32_t uavIndex = registerMap.uavs[stage][reg];
			if( uavIndex < registerMap.uavCount )
			{
				const Resource& resource = m_uavs[uavIndex];
				switch( resource.type )
				{
				case Resource::BUFFER:
					if( resource.buffer.IsValid() && reg < METAL_UAV_BUFFER_COUNT )
					{
						const NSUInteger bufferIndex = METAL_UAV_BUFFER_OFFSET + reg;
						buffers[bufferIndex] = resource.buffer.TrinityALImpl_GetObject()->GetMetalBuffer();
						buffersMask |= ( 1u << bufferIndex );
					}
					break;
				case Resource::TEXTURE:
					if( resource.texture.IsValid() && reg < METAL_UAV_TEXTURE_COUNT )
					{
						id<MTLTexture> uavTexture = resource.texture.TrinityALImpl_GetObject()->GetUAVMetalTexture( resource.mip );
						if( uavTexture )
						{
							const NSUInteger texIndex = METAL_UAV_TEXTURE_OFFSET + reg;
							textures[texIndex] = uavTexture;
							texturesMin = std::min<NSUInteger>( texturesMin, texIndex );
							texturesMax = std::max<NSUInteger>( texturesMax, texIndex );
							missingTextureMask &= ~( 1u << texIndex );
						}
					}
					break;
				case Resource::HEAP_VIEW:
					if( reg < METAL_UAV_BUFFER_COUNT )
					{
						heapViewMask |= ( 1u << ( METAL_UAV_BUFFER_OFFSET + reg ) );
					}
					break;
				default:
					break;
				}
			}

			const uint32_t samplerIndex = registerMap.samplers[stage][reg];
			if( samplerIndex < registerMap.samplerCount )
			{
				const Sampler& sampler = m_samplers[samplerIndex];
				switch( sampler.type )
				{
				case Sampler::SAMPLER:
					if( sampler.sampler.IsValid() && reg < METAL_MAX_BOUND_SAMPLERS )
					{
						samplers[reg] = sampler.sampler.TrinityALImpl_GetObject()->GetMetalSamplerState();
						samplersMin = std::min<NSUInteger>( samplersMin, reg );
						samplersMax = std::max<NSUInteger>( samplersMax, reg );
						missingSamplerMask &= ~( 1u << reg );
					}
					break;
				case Sampler::HEAP_VIEW:
					if( reg < METAL_SRV_BUFFER_COUNT )
					{
						heapViewMask |= ( 1u << ( METAL_SRV_BUFFER_OFFSET + reg ) );
					}
					break;
				default:
					break;
				}
			}
		}

		for( uint32_t index = 0; missingTextureMask && index < METAL_MAX_BOUND_TEXTURES;
			 missingTextureMask >>= 1, ++index )
		{
			if( missingTextureMask & 0x1 )
			{
				textures[index] = metalContext->GetDummyTexture( MTLTextureType( resourceMasks[stage].textureTypes[index] ) );
				texturesMin = std::min<NSUInteger>( texturesMin, index );
				texturesMax = std::max<NSUInteger>( texturesMax, index );
			}
		}

		for( uint32_t index = 0; missingSamplerMask && index < METAL_MAX_BOUND_SAMPLERS;
			 missingSamplerMask >>= 1, ++index )
		{
			if( missingSamplerMask & 0x1 )
			{
				samplers[index] = metalContext->GetDummySampler();
				samplersMin = std::min<NSUInteger>( samplersMin, index );
				samplersMax = std::max<NSUInteger>( samplersMax, index );
			}
		}

		workQueue->SetBuffers( stage, buffers, buffersMask, heapView, heapViewMask );
		workQueue->SetTextures( stage,
								textures,
								( texturesMin != NSUIntegerMax ) ?
									NSMakeRange( texturesMin, texturesMax - texturesMin + 1 ) :
									NSMakeRange( 0, 0 ) );
		workQueue->SetSamplers( stage,
								samplers,
								( samplersMin != NSUIntegerMax ) ?
									NSMakeRange( samplersMin, samplersMax - samplersMin + 1 ) :
									NSMakeRange( 0, 0 ) );
	}

	m_committed = true;
	m_sealed = true;
	return S_OK;
}

#endif
