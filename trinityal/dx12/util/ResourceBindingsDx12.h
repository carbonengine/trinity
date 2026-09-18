// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <vector>

#include "../../ALResult.h"
#include "../../Tr2RenderContextEnum.h"
#include "../../include/Tr2BufferAL.h"
#include "../../include/Tr2RegisterMapAL.h"
#include "../../include/Tr2SamplerStateAL.h"
#include "../../include/Tr2TextureAL.h"

class Tr2RenderContextAL;

namespace TrinityALImpl
{
struct Tr2RootSignatureAL;
}

/** Pending shader resource, unordered access and sampler bindings for a render context */
class ResourceBindings
{
public:
	ResourceBindings();

	ALResult SetSrv( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw();
	ALResult SetSrv( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw();
	ALResult SetUav( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw();
	ALResult SetUav( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw();
	ALResult SetSrvHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	ALResult SetUavHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	ALResult SetSampler( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw();
	ALResult SetSamplerHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();

	/** Drop everything that was set, flushing any pending out transitions */
	ALResult Reset( Tr2RenderContextAL& context ) throw();

	/** Start a new set of bindings once the previous set has been committed */
	void BeginBatch( Tr2RenderContextAL& context ) throw();

	/** Drop everything that was set without touching the command list */
	void Discard() throw();

	/** Force the next Commit to rebind, without dropping what was set */
	void Invalidate() throw();

	/** Bind everything that was set through the root signature of the program about to be used */
	ALResult Commit( Tr2RenderContextAL& context, const TrinityALImpl::Tr2RootSignatureAL& rootSignature ) throw();

	const std::vector<ID3D12Resource*>& GetUsedResources() const
	{
		return m_usedResources;
	}

private:
	struct Resource
	{
		enum Type
		{
			NONE,
			BUFFER,
			TEXTURE,
			HEAP_VIEW,
		};

		Tr2RenderContextEnum::ShaderType stage = Tr2RenderContextEnum::INVALID_SHADER;
		uint32_t registerIndex = 0;
		Tr2TextureAL texture;
		Tr2BufferAL buffer;
		Type type = NONE;
		union
		{
			Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
			uint32_t mip;
		};
	};

	struct Sampler
	{
		enum Type
		{
			NONE,
			SAMPLER,
			HEAP_VIEW,
		};

		Tr2RenderContextEnum::ShaderType stage = Tr2RenderContextEnum::INVALID_SHADER;
		uint32_t registerIndex = 0;
		Tr2SamplerStateAL sampler;
		Type type = NONE;
	};

	void ClearSorted() throw();
	void FlushOutTransitions( Tr2RenderContextAL& context ) throw();

	std::vector<Resource> m_pendingSRVs;
	std::vector<Resource> m_pendingUAVs;
	std::vector<Sampler> m_pendingSamplers;

	const Resource* m_sortedSRVs[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	const Resource* m_sortedUAVs[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	const Sampler* m_sortedSamplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];

	std::vector<D3D12_RESOURCE_BARRIER> m_outTransitions;
	std::vector<ID3D12Resource*> m_usedResources;

	bool m_committed;
	bool m_sealed;
	const TrinityALImpl::Tr2RootSignatureAL* m_committedRootSignature;
};

#endif
