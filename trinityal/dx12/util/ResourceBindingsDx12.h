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

/** Shader resource, unordered access and sampler bindings for a render context, stored in the
	slots of the root signature that was set before them */
class ResourceBindings
{
public:
	ResourceBindings();

	/** Select the root signature the following bindings are for; drops everything set for the previous one */
	void SetRootSignature( const TrinityALImpl::Tr2RootSignatureAL* rootSignature ) throw();

	const TrinityALImpl::Tr2RootSignatureAL* GetRootSignature() const
	{
		return m_rootSignature;
	}

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

	/** Drop everything that was set, without touching the command list */
	void Discard() throw();

	/** Bind everything that was set through the current root signature */
	ALResult Commit( Tr2RenderContextAL& context ) throw();

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

		Tr2SamplerStateAL sampler;
		Type type = NONE;
	};

	/** Start a new set of bindings once the previous set has been committed */
	void BeginBatch( Tr2RenderContextAL& context ) throw();
	void Clear() throw();
	void FlushOutTransitions( Tr2RenderContextAL& context ) throw();
	Resource* GetSrvSlot( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	Resource* GetUavSlot( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	Sampler* GetSamplerSlot( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();

	Resource m_srvs[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	Resource m_uavs[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	Sampler m_samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];

	std::vector<D3D12_RESOURCE_BARRIER> m_outTransitions;
	std::vector<ID3D12Resource*> m_usedResources;

	const TrinityALImpl::Tr2RootSignatureAL* m_rootSignature;
	bool m_committed;
	bool m_sealed;
};

#endif
