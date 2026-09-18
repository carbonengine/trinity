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
	m_committed( false ),
	m_sealed( false ),
	m_committedProgram( nullptr )
{
}

void Tr2ResourceBindings::BeginBatch() throw()
{
	if( !m_sealed )
	{
		return;
	}

	m_sealed = false;
	m_committed = false;

	m_pendingSRVs.clear();
	m_pendingUAVs.clear();
	m_pendingSamplers.clear();
}

void Tr2ResourceBindings::Discard() throw()
{
	m_pendingSRVs.clear();
	m_pendingUAVs.clear();
	m_pendingSamplers.clear();

	std::fill( std::begin( m_sortedSRVs ), std::end( m_sortedSRVs ), nullptr );
	std::fill( std::begin( m_sortedUAVs ), std::end( m_sortedUAVs ), nullptr );
	std::fill( std::begin( m_sortedSamplers ), std::end( m_sortedSamplers ), nullptr );

	m_committed = false;
	m_sealed = false;
	m_committedProgram = nullptr;
}

void Tr2ResourceBindings::Invalidate() throw()
{
	m_committed = false;
}

ALResult Tr2ResourceBindings::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::BUFFER;
	resource.buffer = buffer;
	m_pendingSRVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::TEXTURE;
	resource.texture = texture;
	resource.colorSpace = colorSpace;
	m_pendingSRVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::BUFFER;
	resource.buffer = buffer;
	m_pendingUAVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::TEXTURE;
	resource.texture = texture;
	resource.mip = mip;
	m_pendingUAVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::HEAP_VIEW;
	m_pendingSRVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::HEAP_VIEW;
	m_pendingUAVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Sampler entry;
	entry.stage = stage;
	entry.registerIndex = registerIndex;
	entry.type = Sampler::SAMPLER;
	entry.sampler = sampler;
	m_pendingSamplers.push_back( entry );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch();

	Sampler entry;
	entry.stage = stage;
	entry.registerIndex = registerIndex;
	entry.type = Sampler::HEAP_VIEW;
	m_pendingSamplers.push_back( entry );

	m_committed = false;
	return S_OK;
}

ALResult Tr2ResourceBindings::Commit( Tr2RenderContextAL& context, const TrinityALImpl::Tr2ShaderProgramAL& program ) throw()
{
	if( m_committed && m_committedProgram == &program )
	{
		return S_OK;
	}

	const Tr2RegisterMapAL& registerMap = program.GetRegisterMap();
	const TrinityALImpl::ShaderResourceMask* resourceMasks = program.GetResourceMasks();

	std::fill( std::begin( m_sortedSRVs ), std::end( m_sortedSRVs ), nullptr );
	std::fill( std::begin( m_sortedUAVs ), std::end( m_sortedUAVs ), nullptr );
	std::fill( std::begin( m_sortedSamplers ), std::end( m_sortedSamplers ), nullptr );

	for( const auto& resource : m_pendingSRVs )
	{
		uint32_t index = registerMap.srvs[resource.stage][resource.registerIndex];
		if( index < registerMap.srvCount )
		{
			m_sortedSRVs[index] = &resource;
		}
	}
	for( const auto& resource : m_pendingUAVs )
	{
		uint32_t index = registerMap.uavs[resource.stage][resource.registerIndex];
		if( index < registerMap.uavCount )
		{
			m_sortedUAVs[index] = &resource;
		}
	}
	for( const auto& sampler : m_pendingSamplers )
	{
		uint32_t index = registerMap.samplers[sampler.stage][sampler.registerIndex];
		if( index < registerMap.samplerCount )
		{
			m_sortedSamplers[index] = &sampler;
		}
	}

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
				const Resource* resource = m_sortedSRVs[srvIndex];
				switch( resource ? resource->type : Resource::NONE )
				{
				case Resource::BUFFER:
					if( resource->buffer.IsValid() && reg < METAL_SRV_BUFFER_COUNT )
					{
						const NSUInteger bufferIndex = METAL_SRV_BUFFER_OFFSET + reg;
						buffers[bufferIndex] = resource->buffer.TrinityALImpl_GetObject()->GetMetalBuffer();
						buffersMask |= ( 1u << bufferIndex );
					}
					break;
				case Resource::TEXTURE:
					if( resource->texture.IsValid() && reg < METAL_SRV_TEXTURE_COUNT )
					{
						const NSUInteger texIndex = METAL_SRV_TEXTURE_OFFSET + reg;
						textures[texIndex] = ( resource->colorSpace == Tr2RenderContextEnum::COLOR_SPACE_SRGB ) ?
							resource->texture.TrinityALImpl_GetObject()->GetSRGBViewMetalTexture() :
							resource->texture.TrinityALImpl_GetObject()->GetMetalTexture();
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
				const Resource* resource = m_sortedUAVs[uavIndex];
				switch( resource ? resource->type : Resource::NONE )
				{
				case Resource::BUFFER:
					if( resource->buffer.IsValid() && reg < METAL_UAV_BUFFER_COUNT )
					{
						const NSUInteger bufferIndex = METAL_UAV_BUFFER_OFFSET + reg;
						buffers[bufferIndex] = resource->buffer.TrinityALImpl_GetObject()->GetMetalBuffer();
						buffersMask |= ( 1u << bufferIndex );
					}
					break;
				case Resource::TEXTURE:
					if( resource->texture.IsValid() && reg < METAL_UAV_TEXTURE_COUNT )
					{
						id<MTLTexture> uavTexture = resource->texture.TrinityALImpl_GetObject()->GetUAVMetalTexture( resource->mip );
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
				const Sampler* sampler = m_sortedSamplers[samplerIndex];
				switch( sampler ? sampler->type : Sampler::NONE )
				{
				case Sampler::SAMPLER:
					if( sampler->sampler.IsValid() && reg < METAL_MAX_BOUND_SAMPLERS )
					{
						samplers[reg] = sampler->sampler.TrinityALImpl_GetObject()->GetMetalSamplerState();
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
	m_committedProgram = &program;
	return S_OK;
}

#endif
