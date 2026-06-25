// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "PsoDescription.h"
#include "../Tr2ShaderProgramALDx12.h"
#include "../Tr2VertexLayoutALDx12.h"

namespace
{
const D3D12_RASTERIZER_DESC s_defaultRasterizer = {
	D3D12_FILL_MODE_SOLID,
	D3D12_CULL_MODE_BACK,
	false,
	0,
	0,
	0,
	true,
	false,
	false,
	0,
	D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};

const D3D12_DEPTH_STENCILOP_DESC s_defaultStencilOp = {
	D3D12_STENCIL_OP_KEEP,
	D3D12_STENCIL_OP_KEEP,
	D3D12_STENCIL_OP_KEEP,
	D3D12_COMPARISON_FUNC_ALWAYS
};

const D3D12_DEPTH_STENCIL_DESC s_defaultDepthStencil = {
	true,
	D3D12_DEPTH_WRITE_MASK_ALL,
	D3D12_COMPARISON_FUNC_LESS,
	false,
	D3D12_DEFAULT_STENCIL_READ_MASK,
	D3D12_DEFAULT_STENCIL_WRITE_MASK,
	s_defaultStencilOp,
	s_defaultStencilOp
};

D3D12_BLEND_DESC s_defaultBlend = {
	false,
	false,
	{ false, false, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL }
};
}

namespace TrinityALImpl
{

PSODescription::PSODescription() :
	m_topologyType( D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED ),
	m_blendDesc( s_defaultBlend ),
	m_rasterizerDesc( s_defaultRasterizer ),
	m_depthStencilDesc( s_defaultDepthStencil ),
	m_renderTargetCount( 0 ),
	m_depthStencilFormat( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN ),
	m_vertexStreamMask( 0 )
{
	std::fill( std::begin( m_renderTargetFormats ), std::end( m_renderTargetFormats ), Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN );
	UpdateHash();
}

bool PSODescription::operator==( const PSODescription& other ) const
{
	return m_vertexLayout == other.m_vertexLayout && m_shaderProgram == other.m_shaderProgram && memcmp( GetHashableBlock(), other.GetHashableBlock(), GetHashableBlockSize() ) == 0;
}

PSODescription::operator size_t() const
{
	return m_hash;
}

ALResult PSODescription::CreatePipelineState( ID3D12Device* device, CComPtr<ID3D12PipelineState>& pipelineState ) const
{
	if( m_shaderProgram.m_program->IsComputeProgramDx12() )
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
		memset( &desc, 0, sizeof( desc ) );
		desc.pRootSignature = m_shaderProgram.m_program->m_rootSignature.m_rootSignature;
		desc.CS = m_shaderProgram.m_program->m_CS;

		return device->CreateComputePipelineState( &desc, IID_PPV_ARGS( &pipelineState ) );
	}
	else
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
		memset( &desc, 0, sizeof( desc ) );

		if( m_shaderProgram.IsValid() )
		{
			desc.pRootSignature = m_shaderProgram.m_program->m_rootSignature.m_rootSignature;
			desc.VS = m_shaderProgram.m_program->m_VS;
			desc.PS = m_shaderProgram.m_program->m_PS;
			desc.DS = m_shaderProgram.m_program->m_DS;
			desc.HS = m_shaderProgram.m_program->m_HS;
			desc.GS = m_shaderProgram.m_program->m_GS;
		}
		desc.RasterizerState = m_rasterizerDesc;
		desc.DepthStencilState = m_depthStencilDesc;
		desc.BlendState = m_blendDesc;
		desc.SampleMask = 0xff;
		desc.SampleDesc.Count = 1;

		std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
		if( !m_vertexLayout.m_layout->m_elements.empty() )
		{
			m_vertexLayout.m_layout->PopulateInputLayout( layout, m_shaderProgram.m_program->m_iaInputs );
			if( !layout.empty() )
			{
				desc.InputLayout.NumElements = UINT( layout.size() );
				desc.InputLayout.pInputElementDescs = &layout[0];
			}
		}
		desc.PrimitiveTopologyType = m_topologyType;


		desc.NumRenderTargets = m_renderTargetCount;
		memcpy( desc.RTVFormats, m_renderTargetFormats, sizeof( desc.RTVFormats ) );

		// fill the rest with unknowns
		std::fill( std::begin( desc.RTVFormats ) + m_renderTargetCount, std::end( desc.RTVFormats ), (DXGI_FORMAT)Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN );

		desc.DSVFormat = DXGI_FORMAT( m_depthStencilFormat );
		desc.SampleDesc.Count = m_sampleDesc.samples;
		desc.SampleDesc.Quality = m_sampleDesc.quality;

		return device->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS( &pipelineState ) );
	}
}

void PSODescription::UpdateHash()
{
	m_hash = CcpHashFNV1( GetHashableBlock(), GetHashableBlockSize() );
	const void* ptrs[2] = { m_shaderProgram.m_program.get(), m_vertexLayout.m_layout.get() };
	m_hash = CcpHashFNV1( ptrs, sizeof( ptrs ), unsigned( m_hash ) );
}

size_t PSODescription::GetHashableBlockSize() const
{
	return reinterpret_cast<const uint8_t*>( &m_sampleDesc ) - reinterpret_cast<const uint8_t*>( &m_topologyType ) + sizeof( m_sampleDesc );
}

const void* PSODescription::GetHashableBlock() const
{
	return &m_topologyType;
}

}

#endif