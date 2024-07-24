////////////////////////////////////////////////////////////
//
//    Created:   Augost 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../../ALResult.h"
#include "../../include/Tr2VertexLayoutAL.h"
#include "../../include/Tr2ShaderProgramAL.h"
#include "../../Tr2HalHelperStructures.h"

namespace TrinityALImpl
{
	class PSODescription
	{
	public:
		PSODescription();

		bool operator==( const PSODescription& other ) const;
		operator size_t() const;

		ALResult CreatePipelineState( ID3D12Device* device, CComPtr<ID3D12PipelineState>& pipelineState ) const;
		void UpdateHash();

		::Tr2VertexLayoutAL m_vertexLayout;
		::Tr2ShaderProgramAL m_shaderProgram;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topologyType;
		D3D12_BLEND_DESC m_blendDesc;
		D3D12_RASTERIZER_DESC m_rasterizerDesc;
		D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc;
		uint32_t m_renderTargetCount;
		Tr2RenderContextEnum::PixelFormat m_renderTargetFormats[8];
		Tr2RenderContextEnum::PixelFormat m_depthStencilFormat;
		Tr2MsaaDesc m_sampleDesc;

		uint32_t m_vertexStreamMask;

	private:
		size_t m_hash;
	private:
		size_t GetHashableBlockSize() const;
		const void* GetHashableBlock() const;
	};

}

namespace std {
	template<> struct hash<TrinityALImpl::PSODescription>
	{
		size_t operator()( const TrinityALImpl::PSODescription& p ) const
		{
			return p;
		}
	};
}


#endif