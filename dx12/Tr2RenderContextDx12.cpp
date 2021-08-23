////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2RenderContextDx12.h"
#include "Tr2BufferALDx12.h"
#include "Tr2TextureALDx12.h"
#include "Tr2VertexLayoutALDx12.h"
#include "Tr2ShaderProgramALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2ResourceSetALDx12.h"
#include "Utilities.h"
#include "Tr2FragmentOpSettings.h"
#include "util/AmdExtDevice.h"


CCP_STATS_DECLARE( primitiveCount, "Trinity/AL/primitiveCount", true, CST_COUNTER_HIGH, "Primitive count in DrawPrimitive calls." );
CCP_STATS_DECLARE( vertexCount, "Trinity/AL/vertexCount", true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );
CCP_STATS_DECLARE( sceneDrawcallCount, "Trinity/AL/sceneDrawcallCount", true, CST_COUNTER_LOW, "Number of DrawPrimitive calls." );

namespace
{
	enum
	{
		/*
		* NOTE: Cache heaps store descriptors for batches in flight, so they typically consume less total memory
		*		but can benefit from having a larger page size to reduce the chance of having to hit the allocator
		*		mid frame
		*/

		DESCRIPTOR_CACHE_CRVSRVUAV_PAGE_SIZE = 4096,
		DESCRIPTOR_CACHE_SAMPLER_PAGE_SIZE = 1024,

		/*
		* NOTE: The cache will allocate additional constant buffer pages if it runs out of room but these will be freed
		*		once they are no longer used to ensure the upload heap usage is kept to a minimum. A warning is output
		*		when this occurs, if it happens a lot then consider increasing the default page size.
		*/
		DESCRIPTOR_CACHE_UPLOAD_DEFAULT_PAGE_SIZE = 4 * 1024 * 1024,
		DESCRIPTOR_CACHE_UPLOAD_SPILL_PAGE_SIZE = 2 * 1024 * 1024,

	};


	D3D12_PRIMITIVE_TOPOLOGY s_topologies[] = {
		D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
		D3D_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
		D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
	};

	D3D12_PRIMITIVE_TOPOLOGY_TYPE s_topologyTypes[] = {
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT
	};

	std::pair<uint32_t, uint32_t> s_primitiveToVertexCount[] = {
		std::make_pair( 0, 0 ),
		std::make_pair( 3, 0 ),
		std::make_pair( 1, 2 ),
		std::make_pair( 0, 0 ),
		std::make_pair( 2, 0 ),
		std::make_pair( 1, 1 ),
		std::make_pair( 1, 0 ),
	};

}


Tr2RenderContextAL::Tr2RenderContextAL() throw( )
	:m_primitiveToVertexCount( 0, 0 ),
	m_ownerDevice( nullptr ),
	m_dirtyPso( true ),
	m_readOnlyDepth( false ),
	m_viewport(),
	m_dynamicVBs( 0 ),
	m_dynamicIB( false ),
	m_separateAlphaBlendEnabled( false ),
	m_srgbWriteEnable( false ),
	m_topology( Tr2RenderContextEnum::TOP_TRIANGLES )
{
	std::fill( std::begin( m_vertexBuffers ), std::end( m_vertexBuffers ), VB() );
}

Tr2RenderContextAL::~Tr2RenderContextAL() throw( )
{
	Destroy();
}

void Tr2RenderContextAL::Destroy() throw( )
{
	ResetDx12();

	m_descriptorCache.clear();


	m_ownerDevice = nullptr;
	m_drawUPHelper.Destroy();

	m_commandList = nullptr;
	m_commandList2 = nullptr;
}

bool Tr2RenderContextAL::IsValid() const throw( )
{
	return m_commandList != nullptr;
}

ALResult Tr2RenderContextAL::CreateDx12( ID3D12CommandAllocator* commandAllocator, Tr2PrimaryRenderContextAL& renderContext )
{
	CR_RETURN_HR( renderContext.m_device->CreateCommandList( 
		0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		commandAllocator,
		nullptr, 
		IID_PPV_ARGS( &m_commandList ) ) );
	CR_RETURN_HR( m_commandList->Close() );
	m_commandList.QueryInterface( &m_commandList2 );

	for( uint32_t bufferIdx = 0; bufferIdx < renderContext.GetBackBufferCount(); ++bufferIdx )
	{
		m_descriptorCache.push_back( std::make_shared<DescriptorStateCache>( 
			renderContext.m_device, 
			&renderContext,
			DESCRIPTOR_CACHE_CRVSRVUAV_PAGE_SIZE, 
			DESCRIPTOR_CACHE_SAMPLER_PAGE_SIZE, 
			DESCRIPTOR_CACHE_UPLOAD_DEFAULT_PAGE_SIZE, 
			DESCRIPTOR_CACHE_UPLOAD_SPILL_PAGE_SIZE ) );
	}
	m_ownerDevice = &renderContext;
	return S_OK;
}

Tr2PrimaryRenderContextAL& Tr2RenderContextAL::GetPrimaryRenderContext()
{
	return *m_ownerDevice;
}

Tr2PrimaryRenderContextAL* Tr2RenderContextAL::GetPrimaryRenderContextPointer()
{
	return m_ownerDevice;
}

ALResult Tr2RenderContextAL::BeginScene() throw( )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::EndScene()
{
	return S_OK;
}


ALResult Tr2RenderContextAL::Clear( uint32_t clearFlags, uint32_t color, float depth, uint32_t stencil, uint32_t slot ) throw( )
{
	bool rtClear = false;
	bool dsClear = false;

	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_TARGET )
	{
		if( m_boundRenderTargets[slot].IsValid() )
		{
			rtClear = true;
		}
		else
		{
			return E_INVALIDCALL;
		}
	}
	if( ( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER ) != 0 || ( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_STENCIL ) != 0 )
	{
		if( m_boundDepthStencil.IsValid() )
		{
			dsClear = true;
		}
		else
		{
			return E_INVALIDCALL;
		}
	}

	if( rtClear || dsClear )
	{
		ID3D12Resource* resources[2];
		size_t count = 0;
		if( rtClear )
		{
			resources[count++] = m_boundRenderTargets[slot].m_texture->GetResourceDx12();
		}
		if( dsClear )
		{
			resources[count++] = m_boundDepthStencil.m_texture->GetResourceDx12();
		}
		FlushBarriersDx12( count, resources );

		if( rtClear )
		{
			float f = 1.0f / 255.0f;
			float colorComponents[] = {
				f * (float)(uint8_t)( color >> 16 ),
				f * (float)(uint8_t)( color >> 8 ),
				f * (float)(uint8_t)( color >> 0 ),
				f * (float)(uint8_t)( color >> 24 )
			};

			const std::shared_ptr<RenderTargetViewDx12>& rtv = m_boundRenderTargets[slot].m_texture->GetRtvDescriptorHandleDx12();
			m_commandList->ClearRenderTargetView( rtv->GetHandleCPU(), colorComponents, 0, nullptr );
		}
		if( dsClear )
		{
			D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAGS();
			if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER )
			{
				flags |= D3D12_CLEAR_FLAG_DEPTH;
			}
			if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_STENCIL )
			{
				flags |= D3D12_CLEAR_FLAG_STENCIL;
			}
			m_commandList->ClearDepthStencilView( m_boundDepthStencil.m_texture->m_dsv->GetHandleCPU(), flags, depth, UINT8( stencil ), 0, nullptr );
		}
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetStreamSource(
	uint32_t index,
	const Tr2BufferAL & buffer,
	uint32_t offset,
	uint32_t stride ) throw( )
{
	if( index > 3 )
	{
		return E_INVALIDARG;
	}

	m_vertexBuffers[index].buffer = buffer;
	m_vertexBuffers[index].offset = offset;
	m_vertexBuffers[index].stride = stride;

	if( !buffer.IsValid() || !HasFlag( buffer.GetDesc().cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
	{
		D3D12_VERTEX_BUFFER_VIEW vb;
		if( buffer.IsValid() )
		{
			vb.BufferLocation = buffer.m_buffer->GetGpuView() + offset;
			vb.SizeInBytes = buffer.GetSize() - offset;
			vb.StrideInBytes = stride;
		}
		else
		{
			vb.BufferLocation = 0;
			vb.SizeInBytes = 0;
			vb.StrideInBytes = 0;
		}
		m_commandList->IASetVertexBuffers( index, 1, &vb );
		m_dynamicVBs = m_dynamicVBs & ~( 1 << index );
	}
	else
	{
		m_dynamicVBs = m_dynamicVBs | ( 1 << index );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL& buffer ) throw( )
{
	m_indexBuffer = buffer;

	if( !buffer.IsValid() || !HasFlag( buffer.GetDesc().cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
	{
		D3D12_INDEX_BUFFER_VIEW ib = { buffer.m_buffer->GetGpuView(), buffer.GetSize(), buffer.GetDesc().stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT };
		m_commandList->IASetIndexBuffer( &ib );
		m_dynamicIB = false;
	}
	else
	{
		m_dynamicIB = true;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout ) throw( )
{
	if( !( m_psoDescription.m_vertexLayout == layout ) )
	{
		m_psoDescription.m_vertexLayout = layout;
		m_dirtyPso = true;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderProgram( const Tr2ShaderProgramAL& shader ) throw( )
{
	if( !( m_psoDescription.m_shaderProgram == shader ) )
	{
		m_psoDescription.m_shaderProgram = shader;
		m_dirtyPso = true;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetTopology( Tr2RenderContextEnum::Topology topology ) throw( )
{
	auto dxTopology = s_topologies[topology];
	m_commandList->IASetPrimitiveTopology( dxTopology );
	m_primitiveToVertexCount = s_primitiveToVertexCount[topology];
	auto topologyType = s_topologyTypes[topology];
	if( topologyType != m_psoDescription.m_topologyType )
	{
		m_psoDescription.m_topologyType = s_topologyTypes[topology];
		m_dirtyPso = true;
	}
	m_topology = topology;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value ) throw( )
{
#define HANDLE_STATE( rs, dest )								\
	case rs:													\
		if( dest != static_cast<decltype( dest )>( value ) )	\
		{														\
			dest = static_cast<decltype( dest )>( value );		\
			m_dirtyPso = true;									\
		}														\
		break;

#define HANDLE_STATE_FLT( rs, dest )							\
	case rs:													\
		if( dest != static_cast<decltype( dest )>( *reinterpret_cast<float*>( &value ) ) )	\
		{														\
			dest = static_cast<decltype( dest )>( *reinterpret_cast<float*>( &value ) );	\
			m_dirtyPso = true;									\
		}														\
		break;

#define HANDLE_STATE_DBL( rs, dest1, dest2 )					\
	case rs:													\
		if( dest1 != static_cast<decltype( dest1 )>( value ) || \
			dest2 != static_cast<decltype( dest2 )>( value ) )	\
		{														\
			dest1 = static_cast<decltype( dest1 )>( value );	\
			dest2 = static_cast<decltype( dest2 )>( value );	\
			m_dirtyPso = true;									\
		}														\
		break;

	switch( state )
	{
	HANDLE_STATE( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, m_psoDescription.m_blendDesc.RenderTarget[0].BlendEnable );
	HANDLE_STATE( Tr2RenderContextEnum::RS_SRCBLEND, m_psoDescription.m_blendDesc.RenderTarget[0].SrcBlend );
	HANDLE_STATE( Tr2RenderContextEnum::RS_DESTBLEND, m_psoDescription.m_blendDesc.RenderTarget[0].DestBlend );
	HANDLE_STATE( Tr2RenderContextEnum::RS_BLENDOP, m_psoDescription.m_blendDesc.RenderTarget[0].BlendOp );
	HANDLE_STATE( Tr2RenderContextEnum::RS_SRCBLENDALPHA, m_psoDescription.m_blendDesc.RenderTarget[0].SrcBlendAlpha );
	HANDLE_STATE( Tr2RenderContextEnum::RS_DESTBLENDALPHA, m_psoDescription.m_blendDesc.RenderTarget[0].DestBlendAlpha );
	HANDLE_STATE( Tr2RenderContextEnum::RS_BLENDOPALPHA, m_psoDescription.m_blendDesc.RenderTarget[0].BlendOpAlpha );
	case Tr2RenderContextEnum::RS_COLORWRITEENABLE:
		if( m_psoDescription.m_blendDesc.RenderTarget[0].RenderTargetWriteMask != ( value & 0xf ) )
		{
			m_psoDescription.m_blendDesc.RenderTarget[0].RenderTargetWriteMask = value & 0xf;
			m_dirtyPso = true;
		}
		break;

	HANDLE_STATE( Tr2RenderContextEnum::RS_CULLMODE, m_psoDescription.m_rasterizerDesc.CullMode );
	HANDLE_STATE( Tr2RenderContextEnum::RS_FILLMODE, m_psoDescription.m_rasterizerDesc.FillMode );
	HANDLE_STATE_FLT( Tr2RenderContextEnum::RS_DEPTHBIAS, m_psoDescription.m_rasterizerDesc.DepthBias ); // cppcheck-suppress invalidPointerCast
	HANDLE_STATE_FLT( Tr2RenderContextEnum::RS_ZBIAS, m_psoDescription.m_rasterizerDesc.DepthBias ); // cppcheck-suppress invalidPointerCast
	HANDLE_STATE_FLT( Tr2RenderContextEnum::RS_SLOPESCALEDEPTHBIAS, m_psoDescription.m_rasterizerDesc.SlopeScaledDepthBias ); // cppcheck-suppress invalidPointerCast

	HANDLE_STATE( Tr2RenderContextEnum::RS_ZENABLE, m_psoDescription.m_depthStencilDesc.DepthEnable );
	HANDLE_STATE( Tr2RenderContextEnum::RS_ZFUNC, m_psoDescription.m_depthStencilDesc.DepthFunc );
	HANDLE_STATE( Tr2RenderContextEnum::RS_STENCILENABLE, m_psoDescription.m_depthStencilDesc.StencilEnable );
	HANDLE_STATE_DBL( Tr2RenderContextEnum::RS_STENCILMASK, m_psoDescription.m_depthStencilDesc.StencilReadMask, m_psoDescription.m_depthStencilDesc.StencilWriteMask );
	HANDLE_STATE_DBL( Tr2RenderContextEnum::RS_STENCILFAIL, m_psoDescription.m_depthStencilDesc.FrontFace.StencilFailOp, m_psoDescription.m_depthStencilDesc.BackFace.StencilFailOp );
	HANDLE_STATE_DBL( Tr2RenderContextEnum::RS_STENCILZFAIL, m_psoDescription.m_depthStencilDesc.FrontFace.StencilDepthFailOp, m_psoDescription.m_depthStencilDesc.BackFace.StencilDepthFailOp );
	HANDLE_STATE_DBL( Tr2RenderContextEnum::RS_STENCILPASS, m_psoDescription.m_depthStencilDesc.FrontFace.StencilPassOp, m_psoDescription.m_depthStencilDesc.BackFace.StencilPassOp );
	HANDLE_STATE_DBL( Tr2RenderContextEnum::RS_STENCILFUNC, m_psoDescription.m_depthStencilDesc.FrontFace.StencilFunc, m_psoDescription.m_depthStencilDesc.BackFace.StencilFunc );

	case Tr2RenderContextEnum::RS_ZWRITEENABLE:
	{
		auto mask = ( value != 0 && !m_readOnlyDepth ) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		if( m_psoDescription.m_depthStencilDesc.DepthWriteMask != mask )
		{
			m_psoDescription.m_depthStencilDesc.DepthWriteMask = mask;
			m_dirtyPso = true;
		}
		break;
	}
	case Tr2RenderContextEnum::RS_STENCILREF:
		m_commandList->OMSetStencilRef( value );
		break;

	case Tr2RenderContextEnum::RS_SEPARATEALPHABLENDENABLE:
		if( m_separateAlphaBlendEnabled != ( value != 0 ) )
		{
			m_separateAlphaBlendEnabled = value != 0;
			m_dirtyPso = true;
		}
		break;
	case Tr2RenderContextEnum::RS_SRGBWRITEENABLE:
		if( m_srgbWriteEnable != ( value != 0 ) )
		{
			m_srgbWriteEnable = value != 0;
			
			D3D12_CPU_DESCRIPTOR_HANDLE handles[RENDER_TARGET_COUNT];
			uint32_t count = 0;
			if( GetRenderTargetHandles( handles, count ) )
			{
				D3D12_CPU_DESCRIPTOR_HANDLE dsHandle;
				if( m_boundDepthStencil.IsValid() )
				{
					dsHandle = m_boundDepthStencil.m_texture->m_dsv->GetHandleCPU();
				}
				m_commandList->OMSetRenderTargets( count, handles, FALSE, m_boundDepthStencil.IsValid() ? &dsHandle : nullptr );
				for( uint32_t i = 0; i < RENDER_TARGET_COUNT; ++i )
				{
					if( m_boundRenderTargets[i].IsValid() )
					{
						m_psoDescription.m_renderTargetFormats[i] = m_srgbWriteEnable ? Tr2RenderContextEnum::MakeSrgb( m_boundRenderTargets[i].GetFormat() ) : m_boundRenderTargets[i].GetFormat();
					}
				}
				m_dirtyPso = true;
			}
		}
		break;
	case Tr2RenderContextEnum::RS_ALPHATESTENABLE:
		if( value == 0 )
		{
			break;
		}
		return S_OK; // TODO: this is not ok
	case Tr2RenderContextEnum::RS_ALPHAFUNC:
	case Tr2RenderContextEnum::RS_ALPHAREF:
		return S_OK;
	default:
		return E_NOTIMPL;
	}
	return S_OK;
#undef SET_STATE
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count ) throw( )
{
	while( count-- )
	{
		auto state = *stateValuePairs++;
		auto value = *stateValuePairs++;
		SetRenderState( Tr2RenderContextEnum::RenderState( state ), value );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetResourceSet( const Tr2ResourceSetAL& resourceSet ) throw( )
{
	if( m_resourceSet.IsValid() && !m_resourceSet.m_resourceSet->m_outTransitions.empty() )
	{
		ResourceBarrierDx12( m_resourceSet.m_resourceSet->m_outTransitions.size(), m_resourceSet.m_resourceSet->m_outTransitions.data() );
	}
	m_resourceSet = resourceSet;

	if( m_resourceSet.IsValid() && !m_resourceSet.m_resourceSet->m_inTransitions.empty() )
	{
		ResourceBarrierDx12( m_resourceSet.m_resourceSet->m_inTransitions.size(), m_resourceSet.m_resourceSet->m_inTransitions.data() );
	}
	resourceSet.m_resourceSet->UploadInitialCounts( *this );

	uint32_t bufferIndex = GetPrimaryRenderContextPointer()->GetCurrentBackBufferIndex();
	m_descriptorCache[bufferIndex]->SetSamplers( 0, resourceSet.m_resourceSet->m_samplerCount, resourceSet.m_resourceSet->m_sampler );

	// Because SRVs and UAVs are stacked in the resource slots, this will filter them out into the correct heap setup calls
	// It's not great, but if the system is changed in the future to separate SRVs and UAVs then this is an easy change
	for (uint32_t idx = 0; idx < resourceSet.m_resourceSet->m_resourceCount; ++idx)
	{
		if (resourceSet.m_resourceSet->m_srv[idx] != nullptr)
		{
			CCP_ASSERT(resourceSet.m_resourceSet->m_uav[idx] == nullptr);

			m_descriptorCache[bufferIndex]->SetShaderResources(idx, 1, &resourceSet.m_resourceSet->m_srv[idx]);
		}
		else if (resourceSet.m_resourceSet->m_uav[idx] != nullptr)
		{
			CCP_ASSERT(resourceSet.m_resourceSet->m_srv[idx] == nullptr);

			m_descriptorCache[bufferIndex]->SetUnorderedAccessViews(idx, 1, &resourceSet.m_resourceSet->m_uav[idx]);
		}
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants( const Tr2ConstantBufferAL& buffer, Tr2RenderContextEnum::ShaderType constantType, uint32_t registerIndex, uint32_t ) throw( )
{
	uint32_t bufferIndex = GetPrimaryRenderContextPointer()->GetCurrentBackBufferIndex();
	m_descriptorCache[bufferIndex]->SetConstantBuffers(constantType, registerIndex, *buffer.m_buffer);
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitive( uint32_t, uint32_t startIndex, uint32_t primitiveCount, uint32_t minimumIndex ) throw( )
{
	CR_RETURN_HR( SetAllState() );
	FlushGraphicsBarriersDx12();

	auto vc = m_primitiveToVertexCount.first * primitiveCount + m_primitiveToVertexCount.second;

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

	m_commandList->DrawIndexedInstanced( vc, 1, startIndex, minimumIndex, 0 );
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount ) throw( )
{
	CR_RETURN_HR( SetAllState() );
	FlushGraphicsBarriersDx12();

	auto vc = m_primitiveToVertexCount.first * primitiveCount + m_primitiveToVertexCount.second;

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

	m_commandList->DrawInstanced( vc, 1, startVertex, 0 );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(
	uint32_t,
	uint32_t startIndex,
	uint32_t primitiveCount,
	uint32_t numInstances ) throw( )
{
	CR_RETURN_HR( SetAllState() );
	FlushGraphicsBarriersDx12();

	auto vc = m_primitiveToVertexCount.first * primitiveCount + m_primitiveToVertexCount.second;

	CCP_STATS_ADD( primitiveCount, primitiveCount * numInstances );
	CCP_STATS_ADD( vertexCount, vc * numInstances );
	CCP_STATS_INC( sceneDrawcallCount );

	m_commandList->DrawIndexedInstanced( vc, numInstances, startIndex, 0, 0 );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstancedIndirect( Tr2BufferAL& params, uint32_t offset ) throw( )
{
	auto buffer = params.m_buffer->GetGpuResource();
	if( params.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( buffer, params.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ) );
	}
	
	CR_RETURN_HR( SetAllState() );
	FlushGraphicsBarriersDx12( buffer );

	CCP_STATS_INC( sceneDrawcallCount );

	m_commandList->ExecuteIndirect( m_ownerDevice->m_drawIndexedInstancedIndirect, 1, buffer, offset, nullptr, 0 );
	if( params.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, params.m_buffer->m_defaultState ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawInstancedIndirect( Tr2BufferAL& params, uint32_t offset ) throw( )
{
	auto buffer = params.m_buffer->GetGpuResource();
	if( params.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( buffer, params.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ) );
	}
	CR_RETURN_HR( SetAllState() );
	FlushGraphicsBarriersDx12( buffer );

	m_commandList->ExecuteIndirect( m_ownerDevice->m_drawInstancedIndirect, 1, buffer, offset, nullptr, 0 );
	if( params.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, params.m_buffer->m_defaultState ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ ) throw( )
{
	CR_RETURN_HR( SetAllState() );
	FlushComputeBarriersDx12();

	m_commandList->Dispatch( groupDimX, groupDimY, groupDimZ );
	return S_OK;
}

ALResult Tr2RenderContextAL::RunComputeShaderIndirect( Tr2BufferAL& params, unsigned offset ) throw( )
{
	auto buffer = params.m_buffer->GetGpuResource();
	if( params.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( buffer, params.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ) );
	}
	CR_RETURN_HR( SetAllState() );
	FlushComputeBarriersDx12( buffer );

	m_commandList->ExecuteIndirect( m_ownerDevice->m_dispatchIndirect, 1, buffer, offset, nullptr, 0 );
	if( params.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, params.m_buffer->m_defaultState ) );
	}

	return S_OK;
}


ALResult Tr2RenderContextAL::CopyBufferCounter( Tr2BufferAL& dest, uint32_t destOffset, Tr2BufferAL& src ) throw( )
{
	if( !dest.IsValid() || !src.m_buffer->m_counter )
	{
		return E_INVALIDARG;
	}
	D3D12_RESOURCE_BARRIER barriers[2];
	uint32_t count = 0;
	barriers[count++] = TrinityALImpl::Transition( src.m_buffer->m_counter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE );
	if( dest.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_COPY_DEST )
	{
		barriers[count++] = TrinityALImpl::Transition( dest.m_buffer->GetGpuResource(), dest.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST );
	}
	ResourceBarrierDx12( count, barriers );
	FlushBarriersDx12( dest.m_buffer->GetGpuResource(), src.m_buffer->m_counter );
	m_commandList->CopyBufferRegion( dest.m_buffer->GetGpuResource(), destOffset, src.m_buffer->m_counter, 0, 4 );
	std::swap( barriers[0].Transition.StateAfter, barriers[0].Transition.StateBefore );
	std::swap( barriers[1].Transition.StateAfter, barriers[1].Transition.StateBefore );
	ResourceBarrierDx12( count, barriers );

	return S_OK;
}



ID3D12PipelineState* Tr2RenderContextAL::GetPipelineState()
{
	if( !m_separateAlphaBlendEnabled )
	{
		auto& desc = m_psoDescription.m_blendDesc.RenderTarget[0];
		static D3D12_BLEND alphaBlendRemap[] = {
			D3D12_BLEND_ZERO,
			D3D12_BLEND_ZERO,
			D3D12_BLEND_ONE,
			D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_DEST_ALPHA,
			D3D12_BLEND_INV_DEST_ALPHA,
			D3D12_BLEND_DEST_ALPHA,
			D3D12_BLEND_INV_DEST_ALPHA,
			D3D12_BLEND_SRC_ALPHA_SAT,
			D3D12_BLEND_BLEND_FACTOR,
			D3D12_BLEND_INV_BLEND_FACTOR,

			D3D12_BLEND_SRC1_ALPHA,
			D3D12_BLEND_INV_SRC1_ALPHA,
			D3D12_BLEND_SRC1_ALPHA,
			D3D12_BLEND_INV_SRC1_ALPHA
		};
		desc.BlendOpAlpha = desc.BlendOp;
		desc.SrcBlendAlpha = alphaBlendRemap[desc.SrcBlend];
		desc.DestBlendAlpha = alphaBlendRemap[desc.DestBlend];
	}

	m_psoDescription.UpdateHash();

	auto found = m_ownerDevice->m_pipelineStates.find( m_psoDescription );
	if( found != m_ownerDevice->m_pipelineStates.end() )
	{
		return found->second;
	}

	CComPtr<ID3D12PipelineState> pipelineState;
	CR( m_psoDescription.CreatePipelineState( m_ownerDevice->m_device, pipelineState ) );

	m_ownerDevice->m_pipelineStates[m_psoDescription] = pipelineState;

	return pipelineState;
}

ALResult Tr2RenderContextAL::SetAllState()
{
	if( m_dynamicVBs )
	{
		D3D12_VERTEX_BUFFER_VIEW vb[4];
		for( uint32_t i = 0; i < 4; ++i )
		{
			if( m_vertexBuffers[i].buffer.IsValid() )
			{
				vb[i].BufferLocation = m_vertexBuffers[i].buffer.m_buffer->GetGpuView() + m_vertexBuffers[i].offset;
				vb[i].SizeInBytes = m_vertexBuffers[i].buffer.GetSize() - m_vertexBuffers[i].offset;
				vb[i].StrideInBytes = m_vertexBuffers[i].stride;
			}
			else
			{
				vb[i].BufferLocation = 0;
				vb[i].SizeInBytes = 0;
				vb[i].StrideInBytes = 0;
			}
		}
		m_commandList->IASetVertexBuffers( 0, 4, vb );
	}
	
	if( m_dynamicIB )
	{
		D3D12_INDEX_BUFFER_VIEW ib = { 0, 0, DXGI_FORMAT_UNKNOWN };
		if( m_indexBuffer.m_buffer->IsValid() )
		{
			ib.BufferLocation = m_indexBuffer.m_buffer->GetGpuView();
			ib.SizeInBytes = m_indexBuffer.GetSize();
			ib.Format = m_indexBuffer.GetDesc().stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		}
		m_commandList->IASetIndexBuffer( &ib );
	}

	if( m_dirtyPso )
	{
		auto pso = GetPipelineState();
		if( !pso )
		{
			return E_FAIL;
		}

		m_commandList->SetPipelineState( pso );
		if( m_psoDescription.m_shaderProgram.IsValid() )
		{
			if( m_psoDescription.m_shaderProgram.m_program->IsComputeProgramDx12() )
			{
				m_commandList->SetComputeRootSignature( m_psoDescription.m_shaderProgram.m_program->m_rootSignature );
			}
			else
			{
				m_commandList->SetGraphicsRootSignature( m_psoDescription.m_shaderProgram.m_program->m_rootSignature );
			}
		}
		else
		{
			m_commandList->SetComputeRootSignature( nullptr );
			m_commandList->SetGraphicsRootSignature( nullptr );
		}
		m_dirtyPso = false;
	}

	if( m_psoDescription.m_shaderProgram.IsValid() )
	{
		uint32_t bufferIndex = GetPrimaryRenderContextPointer()->GetCurrentBackBufferIndex();
		m_descriptorCache[bufferIndex]->Commit(m_commandList, m_psoDescription.m_shaderProgram.m_program.get() );
	}

	return S_OK;
}

void Tr2RenderContextAL::FlushGraphicsBarriersDx12( ID3D12Resource* resource )
{
	if( m_barriers.empty() )
	{
		return;
	}
	size_t count = 0;
	// resource + m_boundRenderTargets + m_boundDepthStencil + m_vertexBuffers + m_indexBuffer + m_resourceSet
	ID3D12Resource* resources[1 + RENDER_TARGET_COUNT + 1 + 4 + 1 + Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE]; 
	
	if( resource )
	{
		resources[count++] = resource;
	}
	for( auto rt = std::begin( m_boundRenderTargets ); rt != std::end( m_boundRenderTargets ); ++rt )
	{
		if( rt->IsValid() )
		{
			resources[count++] = rt->m_texture->GetResourceDx12();
		}
	}
	if( m_boundDepthStencil.IsValid() )
	{
		resources[count++] = m_boundDepthStencil.m_texture->GetResourceDx12();
	}
	for( uint32_t i = 0; i < 4; ++i )
	{
		if( m_vertexBuffers[i].buffer.IsValid() )
		{
			resources[count++] = m_vertexBuffers[i].buffer.m_buffer->m_buffer.GetResource();
		}
	}
	if( m_indexBuffer.IsValid() )
	{
		resources[count++] = m_indexBuffer.m_buffer->m_buffer.GetResource();
	}

	if( m_resourceSet.IsValid() )
	{
		std::copy( begin( m_resourceSet.m_resourceSet->m_usedResources ), end( m_resourceSet.m_resourceSet->m_usedResources ), resources + count );
		count += m_resourceSet.m_resourceSet->m_usedResources.size();
	}

	FlushBarriersDx12( count, resources );
}

void Tr2RenderContextAL::FlushComputeBarriersDx12( ID3D12Resource* resource )
{
	if( m_barriers.empty() )
	{
		return;
	}
	if( !resource )
	{
		if( m_resourceSet.IsValid() && !m_resourceSet.m_resourceSet->m_usedResources.empty() )
		{
			FlushBarriersDx12( m_resourceSet.m_resourceSet->m_usedResources.size(), m_resourceSet.m_resourceSet->m_usedResources.data() );
		}
	}
	else
	{
		size_t count = 0;
		ID3D12Resource* resources[1 + Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE];
		resources[count++] = resource;

		if( m_resourceSet.IsValid() && !m_resourceSet.m_resourceSet->m_usedResources.empty() )
		{
			std::copy( begin( m_resourceSet.m_resourceSet->m_usedResources ), end( m_resourceSet.m_resourceSet->m_usedResources ), resources + count );
			count += m_resourceSet.m_resourceSet->m_usedResources.size();
		}

		FlushBarriersDx12( count, resources );
	}
}

ALResult Tr2RenderContextAL::PushRenderTarget( uint32_t slot ) throw()
{
	m_rtStack[slot].push_back( m_boundRenderTargets[slot] );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopRenderTarget( uint32_t slot ) throw()
{
	auto& stack = m_rtStack[slot];
	if( stack.empty() )
	{
		return E_INVALIDCALL;
	}
	auto rt = stack.back();
	stack.pop_back();
	return SetRenderTarget( rt, slot );
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot ) throw( )
{
	if( renderTarget == m_boundRenderTargets[slot] )
	{
		return S_OK;
	}
	if( renderTarget.IsValid() && renderTarget.m_texture->GetRtvDescriptorHandleDx12(m_srgbWriteEnable ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR) == nullptr )
	{
		return E_INVALIDARG;
	}

	D3D12_RESOURCE_BARRIER barriers[2];
	uint32_t barrierCount = 0;

	if( m_boundRenderTargets[slot].IsValid() )
	{
		if( m_boundRenderTargets[slot].m_texture->m_defaultState != D3D12_RESOURCE_STATE_RENDER_TARGET )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( m_boundRenderTargets[slot].m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_RENDER_TARGET, m_boundRenderTargets[slot].m_texture->m_defaultState );
		}
	}
	if( renderTarget.IsValid() )
	{
		if( renderTarget.m_texture->m_defaultState != D3D12_RESOURCE_STATE_RENDER_TARGET )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( renderTarget.m_texture->GetResourceDx12(), renderTarget.m_texture->m_defaultState, D3D12_RESOURCE_STATE_RENDER_TARGET );
		}
	}
	if( barrierCount )
	{
		ResourceBarrierDx12( barrierCount, barriers );
	}
	m_boundRenderTargets[slot] = renderTarget;

	D3D12_CPU_DESCRIPTOR_HANDLE handles[RENDER_TARGET_COUNT];
	uint32_t count = 0;
	if( GetRenderTargetHandles( handles, count ) )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsHandle;
		if( m_boundDepthStencil.IsValid() )
		{
			dsHandle = m_boundDepthStencil.m_texture->m_dsv->GetHandleCPU();
		}
		m_commandList->OMSetRenderTargets( count, handles, FALSE, m_boundDepthStencil.IsValid() ? &dsHandle : nullptr );

		if( m_boundRenderTargets[0].IsValid() )
		{
			SetViewport( Tr2Viewport( m_boundRenderTargets[0].GetWidth(), m_boundRenderTargets[0].GetHeight() ) );
			D3D12_RECT rect = { 0, 0, LONG( m_boundRenderTargets[0].GetWidth() ), LONG( m_boundRenderTargets[0].GetHeight() ) };
			m_commandList->RSSetScissorRects( 1, &rect );
		}
		m_psoDescription.m_renderTargetCount = count;
	}

	m_psoDescription.m_renderTargetFormats[slot] = m_srgbWriteEnable ? Tr2RenderContextEnum::MakeSrgb( renderTarget.GetFormat() ) : renderTarget.GetFormat();
	if( slot == 0 )
	{
		m_psoDescription.m_sampleDesc = renderTarget.GetMsaaDesc();
	}

	m_dirtyPso = true;

	return S_OK;
}


ALResult Tr2RenderContextAL::PushDepthStencil() throw( )
{
	m_dsStack.push_back( m_boundDepthStencil );
	return S_OK;
}
ALResult Tr2RenderContextAL::PopDepthStencil() throw( )
{
	if( m_dsStack.empty() )
	{
		return E_INVALIDCALL;
	}
	auto ds = m_dsStack.back();
	m_dsStack.pop_back();
	return SetDepthStencil( ds );
}

ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2TextureAL& depthStencil ) throw( )
{
	if( depthStencil == m_boundDepthStencil )
	{
		return S_OK;
	}
	if( depthStencil.IsValid() && depthStencil.m_texture->m_dsv == nullptr )
	{
		return E_INVALIDARG;
	}

	D3D12_RESOURCE_BARRIER barriers[2];
	uint32_t barrierCount = 0;

	D3D12_RESOURCE_STATES depthState = m_readOnlyDepth ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;
	if( m_boundDepthStencil.IsValid() )
	{
		if( m_boundDepthStencil.m_texture->m_defaultState != depthState )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( m_boundDepthStencil.m_texture->GetResourceDx12(), depthState, m_boundDepthStencil.m_texture->m_defaultState );
		}
	}
	if( depthStencil.IsValid() )
	{
		if( depthStencil.m_texture->m_defaultState != depthState )
		{
			barriers[barrierCount++] = TrinityALImpl::Transition( depthStencil.m_texture->GetResourceDx12(), depthStencil.m_texture->m_defaultState, depthState );
		}
	}
	if( barrierCount )
	{
		ResourceBarrierDx12( barrierCount, barriers );
	}

	m_boundDepthStencil = depthStencil;

	D3D12_CPU_DESCRIPTOR_HANDLE handles[RENDER_TARGET_COUNT];
	uint32_t count = 0;
	if( GetRenderTargetHandles( handles, count ) )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsHandle;
		if( m_boundDepthStencil.IsValid() )
		{
			dsHandle = m_boundDepthStencil.m_texture->m_dsv->GetHandleCPU();
		}
		m_commandList->OMSetRenderTargets( count, handles, FALSE, m_boundDepthStencil.IsValid() ? &dsHandle : nullptr );

		if( m_boundRenderTargets[0].IsValid() )
		{
			SetViewport( Tr2Viewport( m_boundRenderTargets[0].GetWidth(), m_boundRenderTargets[0].GetHeight() ) );
			D3D12_RECT rect = { 0, 0, LONG( m_boundRenderTargets[0].GetWidth() ), LONG( m_boundRenderTargets[0].GetHeight() ) };
			m_commandList->RSSetScissorRects( 1, &rect );
		}
		m_psoDescription.m_renderTargetCount = count;
	}
	m_psoDescription.m_depthStencilFormat = depthStencil.GetFormat();

	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool enable ) throw( )
{
	if( m_readOnlyDepth == enable )
	{
		return;
	}
	m_readOnlyDepth = enable;
	if( !m_boundDepthStencil.IsValid() )
	{
		return;
	}
	D3D12_RESOURCE_STATES fromState = m_readOnlyDepth ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_DEPTH_READ;
	D3D12_RESOURCE_STATES toState = m_readOnlyDepth ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarrierDx12( TrinityALImpl::Transition( m_boundDepthStencil.m_texture->GetResourceDx12(), fromState, toState ) );

	if( m_readOnlyDepth )
	{
		SetRenderState( Tr2RenderContextEnum::RS_ZWRITEENABLE, 0 );
	}
}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return m_readOnlyDepth;
}

ALResult Tr2RenderContextAL::GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot ) throw( )
{
	if( !m_boundRenderTargets[slot].IsValid() )
	{
		return E_INVALIDARG;
	}
	width = m_boundRenderTargets[slot].GetWidth();
	height = m_boundRenderTargets[slot].GetHeight();
	return S_OK;
}


ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(
	uint32_t numVertices,
	uint32_t primitiveCount,
	const uint32_t* indexData,
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride ) throw( )
{
	return m_drawUPHelper.DrawIndexedPrimitiveUP( m_topology, numVertices, primitiveCount, indexData, vertexStreamZeroData, vertexStreamZeroStride, *this, *m_ownerDevice );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(
	uint32_t numVertices,
	uint32_t primitiveCount,
	const uint16_t* indexData,
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride ) throw( )
{
	return m_drawUPHelper.DrawIndexedPrimitiveUP( m_topology, numVertices, primitiveCount, indexData, vertexStreamZeroData, vertexStreamZeroStride, *this, *m_ownerDevice );
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP(
	uint32_t primitiveCount,
	const void* vertexStreamZeroData,
	uint32_t VertexStreamZeroStride ) throw( )
{
	return m_drawUPHelper.DrawPrimitiveUP( m_topology, primitiveCount, vertexStreamZeroData, VertexStreamZeroStride, *this, *m_ownerDevice );
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport ) throw( )
{
	m_viewport = viewport;
	D3D12_VIEWPORT vp = { viewport.m_x + 0.5f, viewport.m_y + 0.5f, viewport.m_width, viewport.m_height, viewport.m_minZ, viewport.m_maxZ };
	m_commandList->RSSetViewports( 1, &vp );
	return S_OK;
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport ) throw( )
{
	viewport = m_viewport;
	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL& buffer, const float values[4] ) throw( )
{
	auto obj = buffer.m_buffer.get();

	if( !obj->m_clearUav )
	{
		return E_INVALIDARG;
	}

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( obj->GetGpuResource(), obj->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) );
	}
	FlushBarriersDx12( obj->GetGpuResource() );

	m_commandList->ClearUnorderedAccessViewFloat(
		obj->m_clearUav->GetHandleGPU(),
		obj->m_clearUav->GetHandleCPU(),
		obj->GetGpuResource(),
		values,
		0,
		nullptr );

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( obj->GetGpuResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, obj->m_defaultState ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL& buffer, const uint32_t values[4] ) throw( )
{
	auto obj = buffer.m_buffer.get();

	if( !obj->m_clearUav )
	{
		return E_INVALIDARG;
	}

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( obj->GetGpuResource(), obj->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) );
	}
	FlushBarriersDx12( obj->GetGpuResource() );

	m_commandList->ClearUnorderedAccessViewUint(
		obj->m_clearUav->GetHandleGPU(),
		obj->m_clearUav->GetHandleCPU(),
		obj->GetGpuResource(),
		values,
		0,
		nullptr );

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( obj->GetGpuResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, obj->m_defaultState ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2TextureAL& texture, uint32_t mip, const float values[4] ) throw( )
{
	auto obj = texture.m_texture.get();

	if( obj->m_uav.size() <= mip )
	{
		return E_INVALIDARG;
	}
	auto gpuDesc = obj->m_uav[mip]->GetHandleGPU();
	auto cpuDesc = obj->m_uav[mip]->GetHandleCPU();
	auto resource = obj->GetResourceDx12();

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( resource, obj->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) );
	}
	FlushBarriersDx12( resource );

	m_commandList->ClearUnorderedAccessViewFloat(
		gpuDesc, 
		cpuDesc,
		resource,
		values,
		0,
		nullptr );

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, obj->m_defaultState ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2TextureAL& texture, uint32_t mip, const uint32_t values[4] ) throw( )
{
	auto obj = texture.m_texture.get();

	if( obj->m_uav.size() <= mip )
	{
		return E_INVALIDARG;
	}
	auto gpuDesc = obj->m_uav[mip]->GetHandleGPU();
	auto cpuDesc = obj->m_uav[mip]->GetHandleCPU();
	auto resource = obj->GetResourceDx12();

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( resource, obj->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) );
	}
	FlushBarriersDx12( resource );

	m_commandList->ClearUnorderedAccessViewUint(
		gpuDesc,
		cpuDesc,
		resource,
		values,
		0,
		nullptr );

	if( obj->m_defaultState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS )
	{
		ResourceBarrierDx12( TrinityALImpl::Transition( resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, obj->m_defaultState ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetNumberOfLights( uint32_t numLights ) throw( )
{
	Tr2FragmentOpSettings settings;
	settings.m_numLights = numLights;

	void* data;
	CR_RETURN_HR( m_ownerDevice->m_shadowCB.Lock( &data, *this ) );
	memcpy( data, &settings, sizeof( settings ) );
	m_ownerDevice->m_shadowCB.Unlock( *this );

	CR_RETURN_HR( SetConstants( m_ownerDevice->m_shadowCB, Tr2RenderContextEnum::PIXEL_SHADER, 10 ) );

	return S_OK;
}

ALResult Tr2RenderContextAL::CopySubBuffer(
	Tr2BufferAL& dest,
	uint32_t destOffset,
	Tr2BufferAL& src,
	uint32_t offset,
	uint32_t length )
{
	if( !dest.IsValid() || !src.IsValid() )
	{
		return E_INVALIDARG;
	}
	D3D12_RESOURCE_BARRIER barriers[2];
	uint32_t count = 0;
	if( dest.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_COPY_DEST )
	{
		barriers[count++] = TrinityALImpl::Transition( dest.m_buffer->GetGpuResource(), dest.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST );
	}
	if( src.m_buffer->m_defaultState != D3D12_RESOURCE_STATE_COPY_SOURCE )
	{
		barriers[count++] = TrinityALImpl::Transition( src.m_buffer->GetGpuResource(), src.m_buffer->m_defaultState, D3D12_RESOURCE_STATE_COPY_SOURCE );
	}
	if( count )
	{
		ResourceBarrierDx12( count, barriers );
	}
	FlushBarriersDx12( dest.m_buffer->GetGpuResource(), src.m_buffer->GetGpuResource() );
	m_commandList->CopyBufferRegion( dest.m_buffer->GetGpuResource(), destOffset, src.m_buffer->GetGpuResource(), offset, length );
	if( count )
	{
		std::swap( barriers[0].Transition.StateAfter, barriers[0].Transition.StateBefore );
		std::swap( barriers[1].Transition.StateAfter, barriers[1].Transition.StateBefore );
		ResourceBarrierDx12( count, barriers );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::FlushAndSyncDx12()
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	return m_ownerDevice->FlushAndSyncDx12( *this );
}

bool Tr2RenderContextAL::GetRenderTargetHandles( D3D12_CPU_DESCRIPTOR_HANDLE* handles, uint32_t& count )
{
	uint32_t width = 0xffffffff;
	uint32_t height = 0xffffffff;
	Tr2MsaaDesc msaa;
	if( m_boundDepthStencil.IsValid() )
	{
		width = m_boundDepthStencil.GetWidth();
		height = m_boundDepthStencil.GetHeight();
		msaa = m_boundDepthStencil.GetMsaaDesc();
	}
	for( uint32_t i = 0; i < RENDER_TARGET_COUNT; ++i )
	{
		if( m_boundRenderTargets[i].IsValid() )
		{
			if( width != 0xffffffff )
			{
				if( m_boundRenderTargets[i].GetWidth() != width || m_boundRenderTargets[i].GetHeight() != height || m_boundRenderTargets[i].GetMsaaDesc() != msaa )
				{
					return false;
				}
			}
			else
			{
				width = m_boundRenderTargets[i].GetWidth();
				height = m_boundRenderTargets[i].GetHeight();
				msaa = m_boundRenderTargets[i].GetMsaaDesc();
			}
		}
	}

	count = 0;
	bool hasNullRts = false;
	bool hasNullRtsInside = false;
	Tr2TextureAL* boundRtExample = nullptr;
	for( uint32_t i = 0; i < RENDER_TARGET_COUNT; ++i )
	{
		if( m_boundRenderTargets[i].IsValid() )
		{
			const std::shared_ptr<RenderTargetViewDx12>& rtv = m_boundRenderTargets[i].m_texture->GetRtvDescriptorHandleDx12( m_srgbWriteEnable ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
			CCP_ASSERT(rtv != nullptr);
			handles[i] = rtv->GetHandleCPU();
			count = i + 1;
			hasNullRtsInside = hasNullRts;
			boundRtExample = m_boundRenderTargets + i;
		}
		else
		{
			hasNullRts = true;
		}
	}
	if( hasNullRtsInside )
	{
		auto null = m_ownerDevice->GetNullRtHandle( *boundRtExample );
		for( uint32_t i = 0; i < count; ++i )
		{
			if( !m_boundRenderTargets[i].IsValid() )
			{
				handles[i] = null;
			}
		}
	}
	return true;
}

bool Tr2RenderContextAL::IsBoundDx12( const TrinityALImpl::Tr2TextureAL& texture, D3D12_RESOURCE_STATES& boundState )
{
	for( uint32_t i = 0; i < RENDER_TARGET_COUNT; ++i )
	{
		if( *m_boundRenderTargets[i].m_texture == texture )
		{
			boundState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			return true;
		}
	}
	if( *m_boundDepthStencil.m_texture == texture )
	{
		boundState = m_readOnlyDepth ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE;
		return true;
	}
	return false;
}

void Tr2RenderContextAL::ResetDx12()
{
	if( m_resourceSet.IsValid() && !m_resourceSet.m_resourceSet->m_outTransitions.empty() )
	{
		ResourceBarrierDx12( m_resourceSet.m_resourceSet->m_outTransitions.size(), m_resourceSet.m_resourceSet->m_outTransitions.data() );
	}

	for( uint32_t i = 0; i < 4; ++i )
	{
		m_vertexBuffers[i].buffer = Tr2BufferAL();
	}
	m_dynamicVBs = 0;
	m_indexBuffer = Tr2BufferAL();
	m_dynamicIB = false;

	m_resourceSet = Tr2ResourceSetAL();

	m_psoDescription = TrinityALImpl::PSODescription();
	m_topology = Tr2RenderContextEnum::TOP_INVALID;
	m_primitiveToVertexCount = std::make_pair( 0, 0 );

	m_dirtyPso = true;
	std::fill( std::begin( m_boundRenderTargets ), std::end( m_boundRenderTargets ), Tr2TextureAL() );
	for( auto it = begin( m_rtStack ); it != end( m_rtStack ); ++it )
	{
		it->clear();
	}
	m_boundDepthStencil = Tr2TextureAL();
	m_dsStack.clear();

	m_readOnlyDepth = false;
	m_viewport = Tr2Viewport();
	m_separateAlphaBlendEnabled = false;
	m_srgbWriteEnable = false;
}

void Tr2RenderContextAL::AddGpuMarker( const char* marker )
{
	m_ownerDevice->GetMarkerBuffer().PutMarker( m_commandList2, marker );
}

void Tr2RenderContextAL::PushGpuMarker( const char* marker )
{
	if( m_ownerDevice->m_amdExtDeviceObject )
	{
		static_cast<IAmdExtD3DDevice1*>( m_ownerDevice->m_amdExtDeviceObject )->PushMarker( m_commandList, marker );
	}
}

void Tr2RenderContextAL::PopGpuMarker()
{
	if( m_ownerDevice->m_amdExtDeviceObject )
	{
		static_cast<IAmdExtD3DDevice1*>( m_ownerDevice->m_amdExtDeviceObject )->PopMarker( m_commandList );
	}
}

/** Forcibly reset and dirty all descriptor caches (used for explicit synchronization) */
void Tr2RenderContextAL::ResetDescriptorCaches()
{
	for (size_t idx = 0; idx < m_descriptorCache.size(); ++idx)
	{
		m_descriptorCache[idx]->Reset();
	}
}

/** Force the current descriptor cache to dirty if something has modified active heaps */
void Tr2RenderContextAL::DirtyDescriptorCache()
{
	uint32_t bufferIndex = GetPrimaryRenderContextPointer()->GetCurrentBackBufferIndex();
	m_descriptorCache[bufferIndex]->Dirty();
}

void Tr2RenderContextAL::ReApplyStateDx12()
{
	m_dirtyPso = true;
	D3D12_CPU_DESCRIPTOR_HANDLE handles[RENDER_TARGET_COUNT];
	uint32_t count = 0;
	if( GetRenderTargetHandles( handles, count ) )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsHandle;
		if( m_boundDepthStencil.IsValid() )
		{
			dsHandle = m_boundDepthStencil.m_texture->m_dsv->GetHandleCPU();
		}
		m_commandList->OMSetRenderTargets( count, handles, FALSE, m_boundDepthStencil.IsValid() ? &dsHandle : nullptr );
	}
	m_commandList->IASetPrimitiveTopology( s_topologies[m_topology] );
	SetViewport( m_viewport );
	if( m_boundRenderTargets[0].IsValid() )
	{
		D3D12_RECT rect = { 0, 0, LONG( m_boundRenderTargets[0].GetWidth() ), LONG( m_boundRenderTargets[0].GetHeight() ) };
		m_commandList->RSSetScissorRects( 1, &rect );
	}
	DirtyDescriptorCache();
}

void Tr2RenderContextAL::ResourceBarrierDx12( size_t count, const D3D12_RESOURCE_BARRIER* barriers )
{
	for( size_t i = 0; i < count; ++i )
	{
		auto& barrier = barriers[i];
		CCP_ASSERT( barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION );
		CCP_ASSERT( barrier.Transition.pResource != nullptr );

		bool found = false;
		for( auto it = begin( m_barriers ); it != end( m_barriers ); ++it )
		{
			if( it->Type != D3D12_RESOURCE_BARRIER_TYPE_TRANSITION )
			{
				continue;
			}
			if( it->Transition.pResource == barrier.Transition.pResource )
			{
				CCP_ASSERT( it->Transition.StateAfter == barrier.Transition.StateBefore );
				if( it->Transition.StateBefore == barrier.Transition.StateAfter )
				{
					if( ( it->Transition.StateBefore & D3D12_RESOURCE_STATE_UNORDERED_ACCESS ) != 0 )
					{
						D3D12_RESOURCE_BARRIER uavBarrier = { D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE };
						uavBarrier.UAV.pResource = it->Transition.pResource;
						*it = uavBarrier;
					}
					else
					{
						m_barriers.erase( it );
					}
				}
				else
				{
					it->Transition.StateAfter = barrier.Transition.StateAfter;
				}
				found = true;
				break;
			}
		}
		if( !found )
		{
			m_barriers.push_back( barrier );
		}
	}
}

void Tr2RenderContextAL::ResourceBarrierDx12( const D3D12_RESOURCE_BARRIER& barrier )
{
	ResourceBarrierDx12( 1, &barrier );
}

void Tr2RenderContextAL::FlushBarriersDx12()
{
	if( !m_barriers.empty() )
	{
		m_commandList->ResourceBarrier( UINT( m_barriers.size() ), m_barriers.data() );
		m_barriers.clear();
	}
}

void Tr2RenderContextAL::FlushBarriersDx12( ID3D12Resource* resource )
{
	FlushBarriersDx12( 1, &resource );
}

void Tr2RenderContextAL::FlushBarriersDx12( ID3D12Resource* resource1, ID3D12Resource* resource2 )
{
	ID3D12Resource* resources[] = { resource1, resource2 };
	FlushBarriersDx12( 2, resources );
}

void Tr2RenderContextAL::FlushBarriersDx12( size_t count, ID3D12Resource** resources )
{
	auto size = m_barriers.size();
	if( size == 0 )
	{
		return;
	}
	if( size < 8 )
	{
		D3D12_RESOURCE_BARRIER barriers[8];
		size_t barrierCount = 0;

		for( size_t i = 0; i < count; ++i )
		{
			auto resource = resources[i];
			for( size_t j = 0; j < size; ++j )
			{
				auto& barrier = m_barriers[j];
				if( barrier.Transition.pResource == resource )
				{
					barriers[barrierCount++] = barrier;

					m_barriers[j] = m_barriers[size - 1];
					m_barriers.pop_back();
					--size;
					break;
				}
			}
		}
		if( barrierCount )
		{
			m_commandList->ResourceBarrier( UINT( barrierCount ), barriers );
		}
	}
	else
	{
		std::vector<D3D12_RESOURCE_BARRIER> barriers;
		barriers.reserve( m_barriers.size() );
		for( size_t i = 0; i < count; ++i )
		{
			auto resource = resources[i];
			for( size_t j = 0; j < size; ++j )
			{
				auto& barrier = m_barriers[j];
				if( barrier.Transition.pResource == resource )
				{
					barriers.push_back( barrier );

					m_barriers[j] = m_barriers[size - 1];
					m_barriers.pop_back();
					--size;
					break;
				}
			}
		}
		if( !barriers.empty() )
		{
			m_commandList->ResourceBarrier( UINT( barriers.size() ), barriers.data() );
		}
	}
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

#endif