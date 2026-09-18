// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2ResourceBindingsDx11_h_
#define Tr2ResourceBindingsDx11_h_

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2RegisterMapAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"

namespace TrinityALImpl
{
class Tr2ShaderProgramAL;
}

// -------------------------------------------------------------
// Description:
//   Shader resource, unordered access and sampler bindings,
//   stored in the slots of the shader program that was set
//   before them, and the state that is currently set on the
//   device context.
// -------------------------------------------------------------
class Tr2ResourceBindings
{
public:
	Tr2ResourceBindings();

	/** Select the program the following bindings are for; drops everything set for the previous one */
	void SetProgram( const TrinityALImpl::Tr2ShaderProgramAL* program ) throw();

	const TrinityALImpl::Tr2ShaderProgramAL* GetProgram() const
	{
		return m_program;
	}

	ALResult SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw();
	ALResult SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw();
	ALResult SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw();
	ALResult SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw();
	ALResult SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	ALResult SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	ALResult SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw();
	ALResult SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();

	/** Drop everything that was set, without touching the device context */
	ALResult Reset() throw();

	/** Drop everything that was set, and everything we believe is set on the device context */
	void Discard() throw();

	/** Release every shader resource slot, and optionally the unordered access views, on the device context */
	void UnbindShaderResources( ID3D11DeviceContext* context, bool unbindUavs ) throw();

	/** Forget which samplers are set on the device context */
	void ClearBoundSamplers() throw();

	/** Bind everything that was set through the register map of the current program */
	ALResult Commit( ID3D11DeviceContext* context ) throw();

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
	void BeginBatch() throw();
	void Clear() throw();
	void UnbindUnorderedAccessViews( ID3D11DeviceContext* context ) throw();
	Resource* GetSrvSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	Resource* GetUavSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();
	Sampler* GetSamplerSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw();

	Resource m_srvs[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	Resource m_uavs[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	Sampler m_samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];

	ID3D11ShaderResourceView* m_boundSrvs[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	ID3D11SamplerState* m_boundSamplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	uint32_t m_assignedUavOffset;
	uint32_t m_assignedUavCount;
	bool m_assignedPsUavs;

	const TrinityALImpl::Tr2ShaderProgramAL* m_program;
	bool m_committed;
	bool m_sealed;
};

#endif

#endif
