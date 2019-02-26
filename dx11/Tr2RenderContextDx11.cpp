#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2RenderContextDx11.h"
#include "ITr2RenderContextEvents.h"
#include "ALLog.h"

#include "Tr2PrimaryRenderContextDx11.h"
#include "Tr2VertexLayoutALDx11.h"
#include "Tr2SamplerStateALDx11.h"
#include "Tr2ShaderALDx11.h"
#include "Tr2HalHelperStructures.h"
#include "Tr2ShaderProgramALDx11.h"
#include "Tr2ResourceSetALDx11.h"
#include "Tr2BufferALDx11.h"


CCP_STATS_DECLARE( primitiveCount		, "Trinity/AL/primitiveCount"		, true, CST_COUNTER_HIGH, "Primitive count in DrawPrimitive calls." );
CCP_STATS_DECLARE( vertexCount			, "Trinity/AL/vertexCount"			, true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );
CCP_STATS_DECLARE( sceneDrawcallCount	, "Trinity/AL/sceneDrawcallCount"	, true, CST_COUNTER_LOW,  "Number of DrawPrimitive calls." );

CCP_STATS_DECLARE( cbCacheHit	, "Trinity/AL/cbCacheHit"	, true, CST_COUNTER_HIGH, "Number of cache hits for dynamic constant buffers." );
CCP_STATS_DECLARE( cbCacheMiss	, "Trinity/AL/cbCacheMiss"	, true, CST_COUNTER_HIGH, "Number of cache misses for dynamic constant buffers." );


using namespace Tr2RenderContextEnum;

namespace {

Tr2PrimaryRenderContextAL*& GetPrimaryRenderContextPointer()
{
	static Tr2PrimaryRenderContextAL* primaryRenderContext = nullptr;
	return primaryRenderContext;
}

	const D3D11_RENDER_TARGET_BLEND_DESC defaultBlend = 
	{	
		FALSE, 
		D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, 
		D3D11_COLOR_WRITE_ENABLE_ALL 
	};

	const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp = 
	{
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_KEEP,
		D3D11_COMPARISON_ALWAYS
	};

	const D3D11_DEPTH_STENCIL_DESC defaultDepthStencil =
	{
		true,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS,
		false,
		D3D11_DEFAULT_STENCIL_READ_MASK,
		D3D11_DEFAULT_STENCIL_WRITE_MASK,
		defaultStencilOp,
		defaultStencilOp
	};

	const D3D11_RASTERIZER_DESC defaultRasterizer =
	{
		D3D11_FILL_SOLID,
		D3D11_CULL_BACK,
		false,
		0,
		0,
		0,
		true,
		false,
		false,
		false
	};
}

namespace Tr2RenderContextImpl {

#pragma warning( disable: 4100 )
	struct NullContext : ID3D11DeviceContext
	{
		virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
			REFIID riid,
			 __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) { return E_FAIL; }
		virtual ULONG STDMETHODCALLTYPE AddRef( void)  { return S_OK; }
		virtual ULONG STDMETHODCALLTYPE Release( void) { return S_OK; }
		virtual void STDMETHODCALLTYPE GetDevice( ID3D11Device **ppDevice) {}

		virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
			REFGUID guid,
			uint32_t *pDataSize,
			__out_bcount_opt( *pDataSize )  void *pData) 
		{ return S_OK; }

		virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
			REFGUID guid,
			uint32_t DataSize,
			const void *pData) 
		{ return S_OK; }

		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
			/* [annotation] */ 
			REFGUID guid,
			/* [annotation] */ 
			__in_opt  const IUnknown *pData) 
		{ return S_OK; }
		virtual void STDMETHODCALLTYPE VSSetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE PSSetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView *const *ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE PSSetShader( 
			__in_opt  ID3D11PixelShader *pPixelShader,
			ID3D11ClassInstance *const *ppClassInstances,
			uint32_t NumClassInstances) {}
		virtual void STDMETHODCALLTYPE PSSetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState *const *ppSamplers) {}
		virtual void STDMETHODCALLTYPE VSSetShader( 
			__in_opt  ID3D11VertexShader *pVertexShader,
			ID3D11ClassInstance *const *ppClassInstances,
			uint32_t NumClassInstances) {}
		virtual void STDMETHODCALLTYPE DrawIndexed( 
			uint32_t IndexCount,
			uint32_t StartIndexLocation,
			INT BaseVertexLocation) {}
		virtual void STDMETHODCALLTYPE Draw( 
			uint32_t VertexCount,
			uint32_t StartVertexLocation) {}
		virtual HRESULT STDMETHODCALLTYPE Map( 
			ID3D11Resource *pResource,
			uint32_t Subresource,
			D3D11_MAP MapType,
			uint32_t MapFlags,
			D3D11_MAPPED_SUBRESOURCE *pMappedResource){ return E_FAIL; }
		virtual void STDMETHODCALLTYPE Unmap( 
			ID3D11Resource *pResource,
			uint32_t Subresource) {}
		virtual void STDMETHODCALLTYPE PSSetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE IASetInputLayout( 
			__in_opt  ID3D11InputLayout *pInputLayout) {}
		virtual void STDMETHODCALLTYPE IASetVertexBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppVertexBuffers,
			const uint32_t *pStrides,
			const uint32_t *pOffsets) {}
		virtual void STDMETHODCALLTYPE IASetIndexBuffer( 
			__in_opt  ID3D11Buffer *pIndexBuffer,
			DXGI_FORMAT Format,
			uint32_t Offset) {}
		virtual void STDMETHODCALLTYPE DrawIndexedInstanced( 
			uint32_t IndexCountPerInstance,
			uint32_t InstanceCount,
			uint32_t StartIndexLocation,
			INT BaseVertexLocation,
			uint32_t StartInstanceLocation) {}
		virtual void STDMETHODCALLTYPE DrawInstanced( 
			uint32_t VertexCountPerInstance,
			uint32_t InstanceCount,
			uint32_t StartVertexLocation,
			uint32_t StartInstanceLocation) {}
		virtual void STDMETHODCALLTYPE GSSetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE GSSetShader( 
			__in_opt  ID3D11GeometryShader *pShader,
			ID3D11ClassInstance *const *ppClassInstances,
			uint32_t NumClassInstances) {}
		virtual void STDMETHODCALLTYPE IASetPrimitiveTopology( 
			D3D11_PRIMITIVE_TOPOLOGY Topology) {}
		virtual void STDMETHODCALLTYPE VSSetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView *const *ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE VSSetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState *const *ppSamplers) {}
		virtual void STDMETHODCALLTYPE Begin( 
			ID3D11Asynchronous *pAsync) {}
		virtual void STDMETHODCALLTYPE End( 
			ID3D11Asynchronous *pAsync) {}
		virtual HRESULT STDMETHODCALLTYPE GetData( 
			ID3D11Asynchronous *pAsync,
			__out_bcount_opt( DataSize )  void *pData,
			uint32_t DataSize,
			uint32_t GetDataFlags){return E_FAIL; }
		virtual void STDMETHODCALLTYPE SetPredication( 
			__in_opt  ID3D11Predicate *pPredicate,
			BOOL PredicateValue) {}
		virtual void STDMETHODCALLTYPE GSSetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView *const *ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE GSSetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState *const *ppSamplers) {}
		virtual void STDMETHODCALLTYPE OMSetRenderTargets( 
			uint32_t NumViews,
			ID3D11RenderTargetView *const *ppRenderTargetViews,
			__in_opt  ID3D11DepthStencilView *pDepthStencilView) {}
		virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews( 
			uint32_t NumRTVs,
			ID3D11RenderTargetView *const *ppRenderTargetViews,
			__in_opt  ID3D11DepthStencilView *pDepthStencilView,
			uint32_t UAVStartSlot,
			uint32_t NumUAVs,
			ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
			const uint32_t *pUAVInitialCounts) {}
		virtual void STDMETHODCALLTYPE OMSetBlendState( 
			__in_opt  ID3D11BlendState *pBlendState,
			__in_opt  const FLOAT BlendFactor[ 4 ],
			uint32_t SampleMask) {}
		virtual void STDMETHODCALLTYPE OMSetDepthStencilState( 
			__in_opt  ID3D11DepthStencilState *pDepthStencilState,
			uint32_t StencilRef) {}
		virtual void STDMETHODCALLTYPE SOSetTargets( 
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppSOTargets,
			const uint32_t *pOffsets) {}
		virtual void STDMETHODCALLTYPE DrawAuto( void) {}
		virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect( 
			ID3D11Buffer *pBufferForArgs,
			uint32_t AlignedByteOffsetForArgs) {}
		virtual void STDMETHODCALLTYPE DrawInstancedIndirect( 
			ID3D11Buffer *pBufferForArgs,
			uint32_t AlignedByteOffsetForArgs) {}
		virtual void STDMETHODCALLTYPE Dispatch( 
			uint32_t ThreadGroupCountX,
			uint32_t ThreadGroupCountY,
			uint32_t ThreadGroupCountZ) {}
		virtual void STDMETHODCALLTYPE DispatchIndirect( 
			ID3D11Buffer *pBufferForArgs,
			uint32_t AlignedByteOffsetForArgs) {}
		virtual void STDMETHODCALLTYPE RSSetState( 
			__in_opt  ID3D11RasterizerState *pRasterizerState) {}
		virtual void STDMETHODCALLTYPE RSSetViewports( 
			uint32_t NumViewports,
			const D3D11_VIEWPORT *pViewports) {}
		virtual void STDMETHODCALLTYPE RSSetScissorRects( 
			uint32_t NumRects,
			const D3D11_RECT *pRects) {}
		virtual void STDMETHODCALLTYPE CopySubresourceRegion( 
			ID3D11Resource *pDstResource,
			uint32_t DstSubresource,
			uint32_t DstX,
			uint32_t DstY,
			uint32_t DstZ,
			ID3D11Resource *pSrcResource,
			uint32_t SrcSubresource,
			__in_opt  const D3D11_BOX *pSrcBox) {}
		virtual void STDMETHODCALLTYPE CopyResource( 
			ID3D11Resource *pDstResource,
			ID3D11Resource *pSrcResource) {}
		virtual void STDMETHODCALLTYPE UpdateSubresource( 
			ID3D11Resource *pDstResource,
			uint32_t DstSubresource,
			__in_opt  const D3D11_BOX *pDstBox,
			const void *pSrcData,
			uint32_t SrcRowPitch,
			uint32_t SrcDepthPitch) {}
		virtual void STDMETHODCALLTYPE CopyStructureCount( 
			ID3D11Buffer *pDstBuffer,
			uint32_t DstAlignedByteOffset,
			ID3D11UnorderedAccessView *pSrcView) {}
		virtual void STDMETHODCALLTYPE ClearRenderTargetView( 
			ID3D11RenderTargetView *pRenderTargetView,
			const FLOAT ColorRGBA[ 4 ]) {}
		virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint( 
			ID3D11UnorderedAccessView *pUnorderedAccessView,
			const uint32_t Values[ 4 ]) {}
		virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat( 
			ID3D11UnorderedAccessView *pUnorderedAccessView,
			const FLOAT Values[ 4 ]) {}
		virtual void STDMETHODCALLTYPE ClearDepthStencilView( 
			ID3D11DepthStencilView *pDepthStencilView,
			uint32_t ClearFlags,
			FLOAT Depth,
			UINT8 Stencil) {}
		virtual void STDMETHODCALLTYPE GenerateMips( 
			ID3D11ShaderResourceView *pShaderResourceView) {}
		virtual void STDMETHODCALLTYPE SetResourceMinLOD( 
			ID3D11Resource *pResource,
			FLOAT MinLOD) {}
		virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD( 
			ID3D11Resource *pResource){return 0; }
		virtual void STDMETHODCALLTYPE ResolveSubresource( 
			ID3D11Resource *pDstResource,
			uint32_t DstSubresource,
			ID3D11Resource *pSrcResource,
			uint32_t SrcSubresource,
			DXGI_FORMAT Format) {}
		virtual void STDMETHODCALLTYPE ExecuteCommandList( 
			ID3D11CommandList *pCommandList,
			BOOL RestoreContextState) {}
		virtual void STDMETHODCALLTYPE HSSetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView *const *ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE HSSetShader( 
			__in_opt  ID3D11HullShader *pHullShader,
			ID3D11ClassInstance *const *ppClassInstances,
			uint32_t NumClassInstances) {}
		virtual void STDMETHODCALLTYPE HSSetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState *const *ppSamplers) {}
		virtual void STDMETHODCALLTYPE HSSetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE DSSetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView *const *ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE DSSetShader( 
			__in_opt  ID3D11DomainShader *pDomainShader,
			ID3D11ClassInstance *const *ppClassInstances,
			uint32_t NumClassInstances) {}
		virtual void STDMETHODCALLTYPE DSSetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState *const *ppSamplers) {}
		virtual void STDMETHODCALLTYPE DSSetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE CSSetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView *const *ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews( 
			uint32_t StartSlot,
			uint32_t NumUAVs,
			ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
			const uint32_t *pUAVInitialCounts) {}
		virtual void STDMETHODCALLTYPE CSSetShader( 
			__in_opt  ID3D11ComputeShader *pComputeShader,
			ID3D11ClassInstance *const *ppClassInstances,
			uint32_t NumClassInstances) {}
		virtual void STDMETHODCALLTYPE CSSetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState *const *ppSamplers) {}
		virtual void STDMETHODCALLTYPE CSSetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer *const *ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE VSGetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE PSGetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE PSGetShader( 
			ID3D11PixelShader **ppPixelShader,
			ID3D11ClassInstance **ppClassInstances,
			uint32_t *pNumClassInstances) {}
		virtual void STDMETHODCALLTYPE PSGetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState **ppSamplers) {}
		virtual void STDMETHODCALLTYPE VSGetShader( 
			ID3D11VertexShader **ppVertexShader,
			ID3D11ClassInstance **ppClassInstances,
			uint32_t *pNumClassInstances) {}
		virtual void STDMETHODCALLTYPE PSGetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE IAGetInputLayout( 
			ID3D11InputLayout **ppInputLayout) {}
		virtual void STDMETHODCALLTYPE IAGetVertexBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppVertexBuffers,
			uint32_t *pStrides,
			uint32_t *pOffsets) {}
		virtual void STDMETHODCALLTYPE IAGetIndexBuffer( 
			ID3D11Buffer **pIndexBuffer,
			DXGI_FORMAT *Format,
			uint32_t *Offset) {}
		virtual void STDMETHODCALLTYPE GSGetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE GSGetShader( 
			ID3D11GeometryShader **ppGeometryShader,
			ID3D11ClassInstance **ppClassInstances,
			uint32_t *pNumClassInstances) {}
		virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology( 
			D3D11_PRIMITIVE_TOPOLOGY *pTopology) {}
		virtual void STDMETHODCALLTYPE VSGetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE VSGetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState **ppSamplers) {}
		virtual void STDMETHODCALLTYPE GetPredication( 
			ID3D11Predicate **ppPredicate,
			BOOL *pPredicateValue) {}
		virtual void STDMETHODCALLTYPE GSGetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE GSGetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState **ppSamplers) {}
		virtual void STDMETHODCALLTYPE OMGetRenderTargets( 
			uint32_t NumViews,
			ID3D11RenderTargetView **ppRenderTargetViews,
			ID3D11DepthStencilView **ppDepthStencilView) {}
		virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews( 
			uint32_t NumRTVs,
			ID3D11RenderTargetView **ppRenderTargetViews,
			ID3D11DepthStencilView **ppDepthStencilView,
			uint32_t UAVStartSlot,
			uint32_t NumUAVs,
			ID3D11UnorderedAccessView **ppUnorderedAccessViews) {}
		virtual void STDMETHODCALLTYPE OMGetBlendState( 
			ID3D11BlendState **ppBlendState,
			FLOAT BlendFactor[ 4 ],
			uint32_t *pSampleMask) {}
		virtual void STDMETHODCALLTYPE OMGetDepthStencilState( 
			ID3D11DepthStencilState **ppDepthStencilState,
			uint32_t *pStencilRef) {}
		virtual void STDMETHODCALLTYPE SOGetTargets( 
			uint32_t NumBuffers,
			ID3D11Buffer **ppSOTargets) {}
		virtual void STDMETHODCALLTYPE RSGetState( 
			ID3D11RasterizerState **ppRasterizerState) {}
		virtual void STDMETHODCALLTYPE RSGetViewports( 
			uint32_t *pNumViewports,
			D3D11_VIEWPORT *pViewports) {}
		virtual void STDMETHODCALLTYPE RSGetScissorRects( 
			uint32_t *pNumRects,
			D3D11_RECT *pRects) {}
		virtual void STDMETHODCALLTYPE HSGetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE HSGetShader( 
			ID3D11HullShader **ppHullShader,
			ID3D11ClassInstance **ppClassInstances,
			uint32_t *pNumClassInstances) {}
		virtual void STDMETHODCALLTYPE HSGetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState **ppSamplers) {}
		virtual void STDMETHODCALLTYPE HSGetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE DSGetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE DSGetShader( 
			ID3D11DomainShader **ppDomainShader,
			ID3D11ClassInstance **ppClassInstances,
			uint32_t *pNumClassInstances) {}
		virtual void STDMETHODCALLTYPE DSGetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState **ppSamplers) {}
		virtual void STDMETHODCALLTYPE DSGetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE CSGetShaderResources( 
			uint32_t StartSlot,
			uint32_t NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews) {}
		virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews( 
			uint32_t StartSlot,
			uint32_t NumUAVs,
			ID3D11UnorderedAccessView **ppUnorderedAccessViews) {}
		virtual void STDMETHODCALLTYPE CSGetShader( 
			ID3D11ComputeShader **ppComputeShader,
			ID3D11ClassInstance **ppClassInstances,
			uint32_t *pNumClassInstances) {}
		virtual void STDMETHODCALLTYPE CSGetSamplers( 
			uint32_t StartSlot,
			uint32_t NumSamplers,
			ID3D11SamplerState **ppSamplers) {}
		virtual void STDMETHODCALLTYPE CSGetConstantBuffers( 
			uint32_t StartSlot,
			uint32_t NumBuffers,
			ID3D11Buffer **ppConstantBuffers) {}
		virtual void STDMETHODCALLTYPE ClearState( void) {}
		virtual void STDMETHODCALLTYPE Flush( void) {}
		virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType( void)
		{return D3D11_DEVICE_CONTEXT_IMMEDIATE; }
		virtual uint32_t STDMETHODCALLTYPE GetContextFlags( void)
		{ return 0; }
		virtual HRESULT STDMETHODCALLTYPE FinishCommandList( 
			BOOL RestoreDeferredContextState,
			ID3D11CommandList **ppCommandList)
		{ return E_FAIL; }
	} s_nullContext;

}
#pragma warning( default: 4100 )

Tr2RenderContextAL::Tr2RenderContextAL() throw()
	: m_topology( TOP_INVALID )
	, m_lastSetTopology( TOP_INVALID )	
	, m_dirtyFlag( 0 )
	, m_renderTargetHighWaterMark( 1 )
	, m_vertexLayout( nullptr )
	, m_lastSetVertexLayout( nullptr )
	, m_lastSetVertexLayoutVSHash( 0 )
	, m_stackDS( "Tr2RenderContextAL::m_stackDS" )
	, m_useReadOnlyDepthView( false )
	, m_isDepthReadOnly( false )
	, m_isSrgbRenderTarget( false )
	, m_hasHullShader( false )
	, m_previouslyHadHullShader( false )
	, m_psUavsDirtyBegin( sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] ) )
	, m_psUavsDirtyEnd( 0 )
	, m_events( nullptr )
	, m_aftermathContext( nullptr )
{	
	m_context.Attach( &Tr2RenderContextImpl::s_nullContext );
	std::fill_n( m_shaders, int(SHADER_TYPE_COUNT), nullptr );

	static_assert(	D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT >= MAX_RENDER_TARGET, 
					"Bad define" );

	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_stackRT[i].SetName( "Tr2RenderContextAL::m_stackRT" );
	}

	for( unsigned i = 0; i < std::extent<decltype( m_sharedConstantBuffers )>::value; ++i )
	{
		m_sharedConstantBuffers[i].size = 0;
	}

	memset( &m_renderStateEmulation.m_currentBlend, 0, 
			sizeof( m_renderStateEmulation.m_currentBlend ) );
	m_renderStateEmulation.m_currentBlend.RenderTarget[0] = defaultBlend;

	m_renderStateEmulation.m_currentDepthStencil = defaultDepthStencil;
	m_renderStateEmulation.m_currentStencilRef = 0;
	m_renderStateEmulation.m_separateAlphaBlendEnabled = false;

	m_renderStateEmulation.m_currentRasterizer = defaultRasterizer;

	memset( &m_renderStateEmulation.m_fragmentOpSettings, 0, 
			sizeof( m_renderStateEmulation.m_fragmentOpSettings ) );

	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestRef = 0;
	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;

	std::fill( std::begin( m_samplerHashes ), std::end( m_samplerHashes ), 0 );
	std::fill( std::begin( m_resourceHashes ), std::end( m_resourceHashes ), 0 );

	m_allRenderStates[RS_SRGBWRITEENABLE] = 0;
}

Tr2RenderContextAL::~Tr2RenderContextAL()
{
	Destroy();
}

void Tr2RenderContextAL::SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* renderContext )
{
	::GetPrimaryRenderContextPointer() = renderContext;
}

Tr2PrimaryRenderContextAL& Tr2RenderContextAL::GetPrimaryRenderContext()
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() );
	return *GetPrimaryRenderContextPointer();
}

Tr2PrimaryRenderContextAL* Tr2RenderContextAL::GetPrimaryRenderContextPointer()
{
	return ::GetPrimaryRenderContextPointer();
}

void Tr2RenderContextAL::Destroy() throw()
{
	for( unsigned i = 0; i < std::extent<decltype( m_sharedConstantBuffers )>::value; ++i )
	{
		m_sharedConstantBuffers[i].size = 0;
	}

	for( size_t i = 0; i < sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] ); ++i )
	{
		m_pixelShaderUavs[i] = nullptr;
	}

	for( size_t i = 0; i < sizeof( m_pixelShaderUavInitialCounts ) / sizeof( m_pixelShaderUavInitialCounts[0] ); ++i )
	{
		m_pixelShaderUavInitialCounts[i] = (uint32_t)-1;
	}

	m_secondaryDevice11 = nullptr;

	m_secondaryDefaultBackBuffer = Tr2TextureAL();	
	if( m_aftermathContext )
	{
		GFSDK_Aftermath_ReleaseContextHandle( reinterpret_cast<GFSDK_Aftermath_ContextHandle>( m_aftermathContext ) );
		m_aftermathContext = nullptr;
	}
	m_context.Attach( &Tr2RenderContextImpl::s_nullContext );

	m_commandList		= nullptr;
		
	m_boundDepthStencil	= Tr2TextureAL();

	m_topology = TOP_INVALID;
	m_lastSetTopology = TOP_INVALID;

	memset( m_boundRenderTarget, 0, sizeof( m_boundRenderTarget[0] ) * MAX_RENDER_TARGET );

	memset( &m_renderStateEmulation.m_currentBlend, 0, 
			sizeof( m_renderStateEmulation.m_currentBlend ) );
	m_renderStateEmulation.m_currentBlend.RenderTarget[0] = defaultBlend;

	m_renderStateEmulation.m_currentDepthStencil = defaultDepthStencil;
	m_renderStateEmulation.m_currentStencilRef = 0;

	m_renderStateEmulation.m_currentRasterizer = defaultRasterizer;

	memset( &m_renderStateEmulation.m_fragmentOpSettings, 0, 
			sizeof( m_renderStateEmulation.m_fragmentOpSettings ) );

	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestRef = 0;
	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;

	//TODO don't do this if this is a secondary context
	m_renderStateEmulation.s_blendStateCache.clear();
	m_renderStateEmulation.s_depthStencilStateCache.clear();
	m_renderStateEmulation.s_rasterizerCache.clear();

	m_dirtyFlag = 0;
	
	m_fragmentOpBuffer.Destroy();

	//raw pointers only//m_vertexShader.Destroy();
	m_vertexLayout = nullptr;
	m_lastSetVertexLayout = nullptr;
	m_lastSetVertexLayoutVSHash = 0;

	m_drawUP.Destroy();
	
	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		TextureStack stack;
		m_stackRT[i].swap( stack );
	}
	{
		TextureStack stack;
		m_stackDS.swap( stack );
	}

	m_hasHullShader = false;
	m_previouslyHadHullShader = false;

	std::fill_n( m_shaders, int( SHADER_TYPE_COUNT ), nullptr );
	m_psUavsDirtyBegin = sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] );
	m_psUavsDirtyEnd = 0;

	memset( m_allRenderStates, 0xff, sizeof( m_allRenderStates ) );
	m_allRenderStates[RS_SRGBWRITEENABLE] = 0;
	std::fill( std::begin( m_samplerHashes ), std::end( m_samplerHashes ), 0 );
	std::fill( std::begin( m_resourceHashes ), std::end( m_resourceHashes ), 0 );
}

PixelFormat Tr2RenderContextAL::GetBackBufferFormat() const throw()
{
	auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
	return renderContext.GetBackBufferFormat();	
}

ALResult Tr2RenderContextAL::BeginScene() throw()
{ 
	std::fill_n( m_shaders, int( SHADER_TYPE_COUNT ), nullptr );
	std::fill( std::begin( m_samplerHashes ), std::end( m_samplerHashes ), 0 );
	std::fill( std::begin( m_resourceHashes ), std::end( m_resourceHashes ), 0 );

	return S_OK; 
}

bool Tr2RenderContextAL::IsValid() const throw()
{
	return m_context.p != &Tr2RenderContextImpl::s_nullContext;
}

__forceinline uint32_t Tr2RenderContextAL::ComputeVertexCount( uint32_t primitiveCount ) const throw()
{
	switch( m_topology )
	{
	case TOP_TRIANGLES:
		return primitiveCount * 3;

	case TOP_TRIANGLE_STRIP:
		return primitiveCount + 2;

	case TOP_LINES:
		return primitiveCount * 2;

	case TOP_LINE_STRIP:
		return primitiveCount + 1;

	case TOP_POINTS:
	case TOP_TRIANGLE_FAN:
		return primitiveCount;
			
	
	default:
		CCP_ASSERT_M( false, "Unsupported topology" );
		return 0;
	}
}

void Tr2RenderContextAL::ApplyReadOnlyDepth() throw()
{
	bool isSrgbRenderTarget = m_allRenderStates[RS_SRGBWRITEENABLE] != 0;
	if( m_useReadOnlyDepthView != m_isDepthReadOnly || isSrgbRenderTarget != m_isSrgbRenderTarget )
	{
		m_isDepthReadOnly = m_useReadOnlyDepthView;
		m_isSrgbRenderTarget = isSrgbRenderTarget;
		SetRtDsToDevice( MAX_RENDER_TARGET );
	}
};

ALResult Tr2RenderContextAL::DrawIndexedPrimitive(	
	uint32_t,
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t ) throw()
{
	auto vc = ComputeVertexCount( primitiveCount );
	
	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

	if( !ApplyShadowRenderStates() )
	{
		return E_FAIL;
	}

	ApplyReadOnlyDepth();
	m_context->DrawIndexed( vc, startIndex, 0 );
	
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(	
	uint32_t, 
	uint32_t startIndex,	
	uint32_t primitiveCount, 
	uint32_t numInstances ) throw()
{
	auto vc = ComputeVertexCount( primitiveCount );

	CCP_STATS_ADD( primitiveCount, primitiveCount * numInstances );
	CCP_STATS_ADD( vertexCount, vc * numInstances );
	CCP_STATS_INC( sceneDrawcallCount );

	if( !ApplyShadowRenderStates() )
	{
		return E_FAIL;
	}

	ApplyReadOnlyDepth();
	m_context->DrawIndexedInstanced( vc, numInstances, startIndex, 0, 0 );
	
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstancedIndirect( Tr2BufferAL& params, uint32_t offset ) throw()
{
	if( !params.IsValid() )
	{
		return E_FAIL;
	}
	if( !ApplyShadowRenderStates() )
	{
		return E_FAIL;
	}

	ApplyReadOnlyDepth();
	m_context->DrawIndexedInstancedIndirect( params.m_buffer->m_buffer, offset );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawInstancedIndirect( Tr2BufferAL& params, uint32_t offset ) throw()
{
	if( !params.IsValid() )
	{
		return E_FAIL;
	}
	if( !ApplyShadowRenderStates() )
	{
		return E_FAIL;
	}

	ApplyReadOnlyDepth();
	m_context->DrawInstancedIndirect( params.m_buffer->m_buffer, offset );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount ) throw()
{
	auto vc = ComputeVertexCount( primitiveCount );

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

	if( !ApplyShadowRenderStates() )
	{
		return E_FAIL;
	}

	ApplyReadOnlyDepth();
	m_context->Draw( vc, startVertex );
	
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP(	
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride ) throw()
{
	return m_drawUP.DrawPrimitiveUP(	primitiveCount,
										vertexStreamZeroData,
										vertexStreamZeroStride,
										*this );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride) throw()
{
	return m_drawUP.DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride,
											*this );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride) throw()
{
	return m_drawUP.DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride,
											*this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Executes currently bound compute shader.
// Arguments:
//   groupDim# - Dimensions of group grid in three axes
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ ) throw()
{
	if( !m_context )
	{
		return E_FAIL;
	}
	ApplyUavs();
	m_context->Dispatch( groupDimX, groupDimY, groupDimZ );
	return S_OK;
}

ALResult Tr2RenderContextAL::RunComputeShaderIndirect( Tr2BufferAL& indirectParams, unsigned offset ) throw( )
{
	if( !indirectParams.IsValid() )
	{
		return E_FAIL;
	}
	ApplyUavs();

	m_context->DispatchIndirect( indirectParams.m_buffer->m_buffer, offset );
	return S_OK;
}

ALResult Tr2RenderContextAL::CopyBufferCounter( Tr2BufferAL& dest, uint32_t destOffset, Tr2BufferAL& src ) throw( )
{
	if( !m_context )
	{
		return E_INVALIDCALL;
	}
	if( !dest.m_buffer->m_buffer || !src.m_buffer->m_uav )
	{
		return E_INVALIDARG;
	}
	m_context->CopyStructureCount( dest.m_buffer->m_buffer, destOffset, src.m_buffer->m_uav );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants(
									const Tr2ConstantBufferAL& buffer, 
									Tr2RenderContextEnum::ShaderType constantType, 
									uint32_t registerIndex, 
									uint32_t ) throw()
{
	using namespace Tr2RenderContextEnum;

	ID3D11Buffer* bufArray[1] = { buffer.m_buffer };

	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );

	if( buffer.GetUsage() & USAGE_LOCK_FREQUENTLY )
	{
		uint32_t constantDataSize = buffer.GetSize();
		const void* constantData = buffer.m_bufferMirror.get();

		if( constantType < SHADER_TYPE_FIRST || constantType >= SHADER_TYPE_COUNT || registerIndex >= CB_SLOT_COUNT )
		{
			return E_INVALIDARG;
		}

		if( constantDataSize == 0 )
		{
			return S_OK;
		}
		SharedConstantBuffer& cb = m_sharedConstantBuffers[constantType * CB_SLOT_COUNT + registerIndex];
		if( cb.mirror.size() < constantDataSize )
		{
			cb.mirror.resize( "Tr2RenderContextAL.m_sharedConstantBuffers.mirror", constantDataSize );
		}
		if( cb.size == constantDataSize )
		{
			if( memcmp( cb.mirror.get(), constantData, constantDataSize ) == 0 )
			{
				CCP_STATS_INC( cbCacheHit );
				if( &buffer != &nullCB )
				{
					buffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;	// it's been set, so user has the choice again between using Lock or the mirror buffer.
				}
				return S_OK;
			}
		}
		bool alreadySet = cb.size != 0;

		CCP_STATS_INC( cbCacheMiss );
		memcpy( cb.mirror.get(), constantData, constantDataSize );
		cb.size = buffer.GetSize();
		if( !cb.constantBuffer.IsValid() || cb.constantBuffer.GetSize() < constantDataSize )
		{
			auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
			CR_RETURN_HR( cb.constantBuffer.Create( constantDataSize, USAGE_CPU_WRITE, nullptr, renderContext ) );
			alreadySet = false;
		}
		void* data;
		CR_RETURN_HR( cb.constantBuffer.Lock( &data, *this ) );
		memcpy( data, constantData, constantDataSize );
		CR_RETURN_HR( cb.constantBuffer.Unlock( *this ) );

		bufArray[0] = cb.constantBuffer.m_buffer;
		cb.constantBuffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;	// it's been set, so user has the choice again between using Lock or the mirror buffer.

		if( &buffer != &nullCB )
		{
			buffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;	// it's been set, so user has the choice again between using Lock or the mirror buffer.
		}

		if( alreadySet )
		{
			return S_OK;
		}
	}
	else
	{
		if( constantType >= SHADER_TYPE_FIRST && constantType < SHADER_TYPE_COUNT && registerIndex < CB_SLOT_COUNT )
		{
			SharedConstantBuffer& cb = m_sharedConstantBuffers[constantType * CB_SLOT_COUNT + registerIndex];
			cb.size = 0;
		}
	}
	
	switch( constantType )
	{
	case VERTEX_SHADER:
		m_context->VSSetConstantBuffers( registerIndex, 1, bufArray );
		break;	

	case PIXEL_SHADER:
		m_context->PSSetConstantBuffers( registerIndex, 1, bufArray );
		break;

	case COMPUTE_SHADER:
		m_context->CSSetConstantBuffers( registerIndex, 1, bufArray );
		break;

	case GEOMETRY_SHADER:
		m_context->GSSetConstantBuffers( registerIndex, 1, bufArray );
		break;

	case HULL_SHADER:
		m_context->HSSetConstantBuffers( registerIndex, 1, bufArray );
		break;

	case DOMAIN_SHADER:
		m_context->DSSetConstantBuffers( registerIndex, 1, bufArray );
		break;

	default:
		return E_FAIL;
	}

	if( &buffer != &nullCB )
	{
		buffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;	// it's been set, so user has the choice again between using Lock or the mirror buffer.
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::Clear(	
	uint32_t clearFlags,
	uint32_t color, 
	float depth, 
	uint32_t stencil,
	uint32_t slot ) throw()
{
	if( clearFlags & CLEARFLAGS_TARGET )
	{
		ID3D11RenderTargetView*	rtView = 
			m_boundRenderTarget[slot].IsValid()	?	m_boundRenderTarget[slot].m_texture->m_renderTarget[COLOR_SPACE_LINEAR]
										:	slot == 0	? m_secondaryDefaultBackBuffer.m_texture->m_renderTarget[COLOR_SPACE_LINEAR]
														: nullptr;

		if( rtView )
		{
			float f = 1.0f / 255.0f;
			float colorComponents[] = {
				f * (float)(uint8_t)( color >> 16 ),
				f * (float)(uint8_t)( color >> 8 ),
				f * (float)(uint8_t)( color >> 0 ),
				f * (float)(uint8_t)( color >> 24 )
			};
			m_context->ClearRenderTargetView( rtView, colorComponents );
		}
		// else can happen if we get here in the middle of a reset/resize -- no valid RT nor backbuffer bound -- or
		// trying to clear slot > 0 with nothing in it.
	}

	uint32_t d3dFlags = 0;
	if( clearFlags & CLEARFLAGS_ZBUFFER )
	{
		d3dFlags |= D3D11_CLEAR_DEPTH;
	}
	if( clearFlags & CLEARFLAGS_STENCIL )
	{
		d3dFlags |= D3D11_CLEAR_STENCIL;
	}

	if( d3dFlags && m_boundDepthStencil.IsValid() )
	{
		m_context->ClearDepthStencilView( m_boundDepthStencil.m_texture->m_depthStencil[TrinityALImpl::Tr2TextureAL::DepthOption::READ_WRITE],
											d3dFlags, depth, 
											static_cast<UINT8>(stencil) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetRtDsToDevice( uint32_t changedSlot ) throw()
{
	m_currentResourceSet = Tr2ResourceSetAL();
	std::fill( std::begin( m_samplerHashes ), std::end( m_samplerHashes ), 0 );
	std::fill( std::begin( m_resourceHashes ), std::end( m_resourceHashes ), 0 );
	decltype( &ID3D11DeviceContext::VSSetShaderResources ) setResources[] = {
		&ID3D11DeviceContext::VSSetShaderResources,
		&ID3D11DeviceContext::PSSetShaderResources,
		&ID3D11DeviceContext::CSSetShaderResources,
		&ID3D11DeviceContext::GSSetShaderResources,
		&ID3D11DeviceContext::HSSetShaderResources,
		&ID3D11DeviceContext::DSSetShaderResources,
	};

	ID3D11ShaderResourceView* nullViews[16] = { nullptr };

	for( uint32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		( m_context->*( setResources[i] ) )( 0, 16, nullViews );
	}

	ID3D11RenderTargetView*	rtViews[MAX_RENDER_TARGET];
	// Follow the DX9 behavior: null means 'default backbuffer' for slot 0, and 'nothing' for everything else.
	for( uint32_t i = 0; i != m_renderTargetHighWaterMark; ++i )
	{
		auto srgb = m_isSrgbRenderTarget ? COLOR_SPACE_SRGB : COLOR_SPACE_LINEAR;
		if( m_boundRenderTarget[i].IsValid() )
		{
			rtViews[i] = m_boundRenderTarget[i].m_texture->m_renderTarget[srgb];
		}
		else if( i == 0 )
		{
			rtViews[i] = m_secondaryDefaultBackBuffer.m_texture->m_renderTarget[srgb];
		}
		else
		{
			rtViews[i] = nullptr;
		}
	}

	auto& bb = m_boundRenderTarget[0].IsValid() ? m_boundRenderTarget[0] : m_secondaryDefaultBackBuffer;

	// Msaa zero is the same as one, so does need to show up as 'compatible'
	const uint32_t dsMsaaType = m_boundDepthStencil.IsValid() ? std::max( m_boundDepthStencil.GetMsaaDesc().samples, 1u ) : 1;
	const uint32_t bbMsaaType = std::max( bb.GetMsaaDesc().samples, 1u );

	// dont't even bother setting it when the dimensions don't match, it's not gonna work.
	// This happens when we set/push/pop an RT and DS in two separate calls -- there's a point between
	// those two where it's in a bad state.  Silently "works" in DX9, complains in DX11. Fix the spam:
	if( !m_boundDepthStencil.IsValid()	||
		( m_boundDepthStencil.GetDesc().GetWidth()		== bb.GetDesc().GetWidth()		&&
		  m_boundDepthStencil.GetDesc().GetHeight()		== bb.GetDesc().GetHeight()		&&
		  m_boundDepthStencil.GetMsaaDesc().quality == bb.GetMsaaDesc().quality	&&
		  dsMsaaType							== bbMsaaType
		) )
	{		
		ID3D11DepthStencilView* dsView = nullptr;
		if( m_boundDepthStencil.IsValid() )
		{
			if( m_isDepthReadOnly )
			{
				dsView = m_boundDepthStencil.m_texture->m_depthStencil[TrinityALImpl::Tr2TextureAL::DepthOption::READ_ONLY];
			}
			else
			{
				dsView = m_boundDepthStencil.m_texture->m_depthStencil[TrinityALImpl::Tr2TextureAL::DepthOption::READ_WRITE];
			}
		}
		if( m_psUavsDirtyBegin < m_psUavsDirtyEnd )
		{
			m_context->OMSetRenderTargetsAndUnorderedAccessViews(	
							m_renderTargetHighWaterMark, 
							rtViews,
							dsView,
							m_psUavsDirtyBegin, 
							m_psUavsDirtyEnd - m_psUavsDirtyBegin,
							reinterpret_cast<ID3D11UnorderedAccessView**>( m_pixelShaderUavs + m_psUavsDirtyBegin ), 
							m_pixelShaderUavInitialCounts + m_psUavsDirtyBegin );
			m_psUavsDirtyBegin = sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] );
			m_psUavsDirtyEnd = 0;
		}
		else
		{
			m_context->OMSetRenderTargetsAndUnorderedAccessViews(	
							m_renderTargetHighWaterMark, 
							rtViews,
							dsView,
							0, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, nullptr, nullptr );
		}
	}

	// emulate DX9 -- setting RT sets the VP, trinity relies on this
	// Note we do this always, even if we didn't change the device at this point because we don't have the matching depthStencil yet.
	// The assumption is that we'll get it, eventually.
	// In other words, avoid a dependency on the order in which people set RT/DS. Without this, RT+DS would be no viewport change,
	// versus DS+RT = would update the viewport.
	//
	// Similar with ScissorRect, see
	//	http://msdn.microsoft.com/en-us/library/windows/desktop/bb147354%28v=vs.85%29.aspx
	// " IDirect3DDevice9::SetRenderTarget resets the scissor rectangle to the full render target, analogous to the viewport reset. "
	//
	if( changedSlot == 0 )
	{
		SetViewport( Tr2Viewport ( bb.GetDesc().GetWidth(), bb.GetDesc().GetHeight() ) );
		SetScissorRect( 0, 0, bb.GetDesc().GetWidth(), bb.GetDesc().GetHeight() );
	}

	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool enable ) throw()
{
	if( m_useReadOnlyDepthView == enable )
	{
		return;
	}

	m_useReadOnlyDepthView = enable;
	m_isDepthReadOnly = enable;
	SetRtDsToDevice( MAX_RENDER_TARGET );
}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return m_useReadOnlyDepthView;
}

ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2TextureAL& depthStencil ) throw()
{
	if( depthStencil.IsValid() )
	{
		if( Tr2GpuUsage::HasFlag( depthStencil.GetGpuUsage(), Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			m_boundDepthStencil = depthStencil;
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		m_boundDepthStencil = Tr2TextureAL();
	}
	
	SetRtDsToDevice( MAX_RENDER_TARGET );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot ) throw()
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	if( renderTarget.IsValid() )
	{
		if( Tr2GpuUsage::HasFlag( renderTarget.GetGpuUsage(), Tr2GpuUsage::RENDER_TARGET ) )
		{
			m_boundRenderTarget[slot] = renderTarget;
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		m_boundRenderTarget[slot] = Tr2TextureAL();
	}

	m_renderTargetHighWaterMark = std::max( m_renderTargetHighWaterMark, slot + 1 );

	SetRtDsToDevice( slot );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetStreamSource(
	uint32_t stream,
	const Tr2BufferAL & buffer,
	uint32_t offset,
	uint32_t stride ) throw()
{
	if( stream == VERTEX_BUFFER_ZERO_STREAM_RESERVED )
	{
		CCP_AL_LOGWARN( "Changing stream %d, which is reserved by Trinity. Undefined behavior.", stream );
	}
	ID3D11Buffer* bufArray[1] = { buffer.m_buffer->m_buffer };
	m_context->IASetVertexBuffers( stream, 1, bufArray, &stride, &offset );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL & buffer ) throw()
{
	m_context->IASetIndexBuffer( buffer.m_buffer->m_buffer,
		buffer.GetDesc().stride == 2  ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
		0 );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetUav(
	Tr2RenderContextEnum::ShaderType inputType,
	uint32_t slot,
	const Tr2BufferAL& buffer,
	uint32_t initialCount ) throw()
{
	ID3D11UnorderedAccessView* view = buffer.m_buffer->m_uav;

	switch( inputType )
	{
	case PIXEL_SHADER:
		if( slot >= sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] ) )
		{
			return E_INVALIDARG;
		}
		m_pixelShaderUavs[slot] = view;
		if( m_psUavsDirtyBegin > slot )
		{
			m_psUavsDirtyBegin = slot;
		}
		if( m_psUavsDirtyEnd < slot + 1 )
		{
			m_psUavsDirtyEnd = slot + 1;
		}
		m_pixelShaderUavInitialCounts[slot] = initialCount;
		break;
	case COMPUTE_SHADER:
		m_context->CSSetUnorderedAccessViews( slot, 1, &view, &initialCount );
		break;
	default:
		return E_INVALIDARG;
	}


	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL& buffer, const float values[4] ) throw()
{
	m_context->ClearUnorderedAccessViewFloat( buffer.m_buffer->m_uav, values );
	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL& buffer, const uint32_t values[4] ) throw()
{
	m_context->ClearUnorderedAccessViewUint( buffer.m_buffer->m_uav, values );
	return S_OK;
}

ALResult Tr2RenderContextAL::CopySubBuffer(
	Tr2BufferAL& dest,
	uint32_t destOffset,
	Tr2BufferAL& src,
	uint32_t offset,
	uint32_t length )
{
	if( !IsValid() || !dest.IsValid() || !src.IsValid() )
	{
		return E_FAIL;
	}
	D3D11_BOX srcBox = { offset, 0, 0, offset + length, 1, 1, };
	m_context->CopySubresourceRegion( dest.m_buffer->m_buffer,
		0,
		destOffset,
		0,
		0,
		src.m_buffer->m_buffer,
		0,
		&srcBox );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout ) throw()
{
	if( !layout.IsValid() && &layout != &nullVL )
	{
		return E_INVALIDARG;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( layout );
	m_vertexLayout = &layout == &nullVL ? nullptr : &layout;

	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderProgram( const Tr2ShaderProgramAL& program ) throw()
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( program );

	if( m_shaders[VERTEX_SHADER] != program.m_shaders[VERTEX_SHADER] )
	{
		m_context->VSSetShader( program.m_shaders[VERTEX_SHADER]->m_shader.vertexShader, nullptr, 0 );
		m_shaders[VERTEX_SHADER] = program.m_shaders[VERTEX_SHADER];
	}
	if( m_shaders[COMPUTE_SHADER] != program.m_shaders[COMPUTE_SHADER] )
	{
		m_context->CSSetShader( program.m_shaders[COMPUTE_SHADER]->m_shader.computeShader, nullptr, 0 );
		m_shaders[COMPUTE_SHADER] = program.m_shaders[COMPUTE_SHADER];
	}
	if( m_shaders[GEOMETRY_SHADER] != program.m_shaders[GEOMETRY_SHADER] )
	{
		m_context->GSSetShader( program.m_shaders[GEOMETRY_SHADER]->m_shader.geometryShader, nullptr, 0 );
		m_shaders[GEOMETRY_SHADER] = program.m_shaders[GEOMETRY_SHADER];
	}
	if( m_shaders[HULL_SHADER] != program.m_shaders[HULL_SHADER] )
	{
		m_context->HSSetShader( program.m_shaders[HULL_SHADER]->m_shader.hullShader, nullptr, 0 );
		m_shaders[HULL_SHADER] = program.m_shaders[HULL_SHADER];
	}
	if( m_shaders[DOMAIN_SHADER] != program.m_shaders[DOMAIN_SHADER] )
	{
		m_context->DSSetShader( program.m_shaders[DOMAIN_SHADER]->m_shader.domainShader, nullptr, 0 );
		m_shaders[DOMAIN_SHADER] = program.m_shaders[DOMAIN_SHADER];
	}
	if( m_shaders[PIXEL_SHADER] != program.m_shaders[PIXEL_SHADER] )
	{
		m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_PATCH_PS;
		m_shaders[PIXEL_SHADER] = program.m_shaders[PIXEL_SHADER];
	}

	m_hasHullShader = program.m_shaders[HULL_SHADER]->m_shader.hullShader != nullptr;

	return S_OK;
}

ALResult Tr2RenderContextAL::SetSamplerState( 
	const Tr2SamplerStateAL& samplerState, 
	ShaderType inputType, 
	uint32_t registerNumber ) throw()
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( *samplerState.m_sampler );
	auto ss = samplerState.m_sampler->m_samplerState.p;
	switch( inputType )
	{
	case VERTEX_SHADER:
		m_context->VSSetSamplers( registerNumber, 1, &ss );
		return S_OK;
	case PIXEL_SHADER:
		m_context->PSSetSamplers( registerNumber, 1, &ss );
		return S_OK;
	case COMPUTE_SHADER:
		m_context->CSSetSamplers( registerNumber, 1, &ss );
		return S_OK;
	case GEOMETRY_SHADER:
		m_context->GSSetSamplers( registerNumber, 1, &ss );
		return S_OK;
	case HULL_SHADER:
		m_context->HSSetSamplers( registerNumber, 1, &ss );
		return S_OK;
	case DOMAIN_SHADER:
		m_context->DSSetSamplers( registerNumber, 1, &ss );
		return S_OK;
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2RenderContextAL::SetRenderState( RenderState state, uint32_t value ) throw()
{
	uint32_t sv[2] = { (uint32_t)state, value };
	return SetRenderStatesImpl( sv, 1 );
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t *stateValuePairs, uint32_t count ) throw()
{
	if( !stateValuePairs )
	{
		return S_OK;
	}

	return SetRenderStatesImpl( stateValuePairs, count );
}

ALResult Tr2RenderContextAL::SetRenderStatesImpl( const uint32_t *stateValuePairs, uint32_t count ) throw()
{
	auto& rt0 = m_renderStateEmulation.m_currentBlend.RenderTarget[0];
	auto& ds  = m_renderStateEmulation.m_currentDepthStencil;
	auto& rs  = m_renderStateEmulation.m_currentRasterizer;
	auto& fos = m_renderStateEmulation.m_fragmentOpSettings;
	auto& atp = m_renderStateEmulation.m_alphaTestParameters;

#define checkBlend(blend,value)								\
	{														\
		if( blend != static_cast<D3D11_BLEND>( value ) )	\
		{													\
			blend = static_cast<D3D11_BLEND>( value );		\
			m_dirtyFlag |= fos.DIRTY_BLEND;					\
		}													\
	}

#define checkBlendOp(op, value )							\
	{														\
		if( op != static_cast<D3D11_BLEND_OP>( value ) )	\
		{													\
			op = static_cast<D3D11_BLEND_OP>( value );		\
			m_dirtyFlag |= fos.DIRTY_BLEND;					\
		}													\
	}

	for(	uint32_t i = 0; 
			( count == 0 && *stateValuePairs ) || ( count != 0 && i != count ); 
			++i )
	{
		static_assert(	sizeof( RenderState ) == sizeof( uint32_t ), 
						"RenderState and value differ in size" );
		const RenderState state = static_cast<RenderState>( *stateValuePairs++ );
		const uint32_t value    = *stateValuePairs++;

		if( (uint32_t)state >= RS_MAX_STATE || m_allRenderStates[ state ] == value )
		{
			continue;
		}

#if 1
		if( state < RS_MAX_STATE )
		{
			m_allRenderStates[ state ] = value;
		}
#endif

		const uint32_t dirty = fos.SetRenderState( state, value, atp );
		if( dirty == fos.HANDLED_BUT_NO_CHANGES )
		{
			continue;	//return S_OK;
		}
		if( dirty )
		{
			m_dirtyFlag |= dirty;
			continue;	//return S_OK;
		}

		

		switch( state )
		{
			// ------------------------------ Alpha blending
		case RS_ALPHABLENDENABLE:
			if( ( rt0.BlendEnable == 0 ) != ( value == 0 ) )
			{
				rt0.BlendEnable = value ? 1 : 0;
				m_dirtyFlag |= fos.DIRTY_BLEND;
			}
			continue;	//return S_OK;

		case RS_SRCBLEND:		/*return*/ checkBlend( rt0.SrcBlend, value );		continue;
		case RS_DESTBLEND:		/*return*/ checkBlend( rt0.DestBlend, value );		continue;
		case RS_SRCBLENDALPHA:	/*return*/ checkBlend( rt0.SrcBlendAlpha, value );	continue;
		case RS_DESTBLENDALPHA:	/*return*/ checkBlend( rt0.DestBlendAlpha, value );	continue;

		case RS_BLENDOP:		/*return*/ checkBlendOp( rt0.BlendOp, value );		continue;
		case RS_BLENDOPALPHA:	/*return*/ checkBlendOp( rt0.BlendOpAlpha, value );	continue;
		
		case RS_COLORWRITEENABLE:
			if( rt0.RenderTargetWriteMask != value )
			{
				rt0.RenderTargetWriteMask = static_cast<UINT8>( value ) & 0xf;
				m_dirtyFlag |= fos.DIRTY_BLEND;			
			}
			//m_queryableRenderState.m_colorWriteEnable = value;
			continue;	//return S_OK;

		case RS_SEPARATEALPHABLENDENABLE:
			if( m_renderStateEmulation.m_separateAlphaBlendEnabled != ( value != 0 ) )
			{
				m_renderStateEmulation.m_separateAlphaBlendEnabled = value != 0;
				m_dirtyFlag |= fos.DIRTY_BLEND;
			}
			continue;	//return S_OK;

			// ------------------------------ Depth Stencil state
		case RS_ZENABLE:
			if( ( ds.DepthEnable != 0 ) != ( value != 0 ) )
			{
				ds.DepthEnable = value ? 1 : 0;
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;			
			}
			//m_queryableRenderState.m_zEnable = value;
			continue;	//return S_OK;

		case RS_ZWRITEENABLE:
			if( ds.DepthWriteMask != ( value	? D3D11_DEPTH_WRITE_MASK_ALL 
												: D3D11_DEPTH_WRITE_MASK_ZERO ) )
			{
				ds.DepthWriteMask = value	? D3D11_DEPTH_WRITE_MASK_ALL 
											: D3D11_DEPTH_WRITE_MASK_ZERO;
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;			
			}
			//m_queryableRenderState.m_zWriteEnable = value;
			continue;	//return S_OK;

		case RS_ZFUNC:
			if( ds.DepthFunc != static_cast<D3D11_COMPARISON_FUNC>( value ) )
			{
				ds.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>( value );	// same -- TODO Halify the enum values
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;

		case RS_STENCILENABLE:
			if( ( ds.StencilEnable == 0 ) != ( value == 0 ) )
			{
				ds.StencilEnable = value ? 1 : 0;
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;

		case RS_STENCILMASK:
			if( ds.StencilReadMask != value || ds.StencilWriteMask != value )
			{
				ds.StencilReadMask = ds.StencilWriteMask = static_cast<UINT8>( value );
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;

		case RS_STENCILREF:
			if( m_renderStateEmulation.m_currentStencilRef != value )
			{
				m_renderStateEmulation.m_currentStencilRef = value;
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;

		case RS_STENCILFAIL:
			if( ds.FrontFace.StencilFailOp != static_cast<D3D11_STENCIL_OP>( value ) ||
				ds.BackFace .StencilFailOp != static_cast<D3D11_STENCIL_OP>( value ) )
			{
				ds.FrontFace.StencilFailOp = 
					ds.BackFace.StencilFailOp = static_cast<D3D11_STENCIL_OP>( value );	// same -- TODO halify
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;

		case RS_STENCILZFAIL:
			if( ds.FrontFace.StencilDepthFailOp != static_cast<D3D11_STENCIL_OP>( value ) ||
				ds.BackFace .StencilDepthFailOp != static_cast<D3D11_STENCIL_OP>( value ) )
			{
				ds.FrontFace.StencilDepthFailOp = 
					ds.BackFace.StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>( value );	// same -- TODO halify
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;

		case RS_STENCILPASS:
			if( ds.FrontFace.StencilPassOp != static_cast<D3D11_STENCIL_OP>( value ) ||
				ds.BackFace .StencilPassOp != static_cast<D3D11_STENCIL_OP>( value ) )
			{
				ds.FrontFace.StencilPassOp = 
					ds.BackFace.StencilPassOp = static_cast<D3D11_STENCIL_OP>( value );	// same -- TODO halify
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;
		
		case RS_STENCILFUNC:
			if( ds.FrontFace.StencilFunc != static_cast<D3D11_COMPARISON_FUNC>( value ) ||
				ds.BackFace .StencilFunc != static_cast<D3D11_COMPARISON_FUNC>( value ) )
			{
				ds.FrontFace.StencilFunc = 
					ds.BackFace.StencilFunc = static_cast<D3D11_COMPARISON_FUNC>( value );	// same -- TODO halify
				m_dirtyFlag |= fos.DIRTY_DEPTHSTENCIL;
			}
			continue;	//return S_OK;


			// ------------------------------ Rasterizer state
		case RS_FILLMODE:
			if( rs.FillMode != static_cast<D3D11_FILL_MODE>( value ) )
			{
				rs.FillMode = static_cast<D3D11_FILL_MODE>( value );	// same -- Halify
				m_dirtyFlag |= fos.DIRTY_RASTERIZER;
			}
			continue;	//return S_OK;

		case RS_CULLMODE:
			{
				auto newValue = value == 1 /* D3DCULL_NONE */	? D3D11_CULL_NONE 
														: ( value == 3 /* D3DCULL_CCW */	? D3D11_CULL_BACK 
																					: D3D11_CULL_FRONT );
				if( rs.CullMode != newValue )
				{
					rs.CullMode = newValue;
					rs.FrontCounterClockwise = FALSE;
					m_dirtyFlag |= fos.DIRTY_RASTERIZER;
				}
				//m_queryableRenderState.m_cullMode = value;
			}
			continue;	//return S_OK;

	
		case RS_DEPTHBIAS:
		case RS_ZBIAS:	// same thing from Dx8?
			if( rs.DepthBias != static_cast<INT>(value) )
			{
				//rs.DepthBias = static_cast<INT>(value);
				rs.DepthBias = static_cast<INT>(*(float*)&value);
				m_dirtyFlag |= fos.DIRTY_RASTERIZER;
			}
			continue;	//return S_OK;

		case RS_SLOPESCALEDEPTHBIAS:
			if( rs.SlopeScaledDepthBias != *((float*)&value) )
			{
				rs.SlopeScaledDepthBias = *((float*)&value);	// undo F2DW
				m_dirtyFlag |= fos.DIRTY_RASTERIZER;
			}
			continue;	//return S_OK;

		case RS_SCISSORTESTENABLE:
			if( ( rs.ScissorEnable == 0 ) != ( value == 0 ) )
			{
				rs.ScissorEnable = value ? 1 : 0;
				m_dirtyFlag |= fos.DIRTY_RASTERIZER;
			}
			continue;	//return S_OK;

		// explicit warning about fixed function requirements
	#define CASE_WARN(x)	\
		case x:				\
			CCP_AL_LOGWARN( "No DX11 support for fixed function state " #x );	continue;	//return S_OK;
	
		CASE_WARN(RS_LIGHTING)
		CASE_WARN(RS_AMBIENT)
		CASE_WARN(RS_FOGVERTEXMODE)
		CASE_WARN(RS_COLORVERTEX)
		CASE_WARN(RS_LOCALVIEWER)
		CASE_WARN(RS_NORMALIZENORMALS)
		CASE_WARN(RS_DIFFUSEMATERIALSOURCE)
		CASE_WARN(RS_SPECULARMATERIALSOURCE)
		CASE_WARN(RS_AMBIENTMATERIALSOURCE)
		CASE_WARN(RS_EMISSIVEMATERIALSOURCE)
	#undef CASE_WARN

		default:
			CCP_AL_LOGWARN( "No DX11 support for SetRenderState( %d, %d )", (uint32_t)state, value );
		}
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetClipPlane( uint32_t planeIndex, const float* planeEq ) throw()
{
	m_dirtyFlag |= m_renderStateEmulation.m_fragmentOpSettings.SetClipPlane( planeIndex, planeEq );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetScissorRect( uint32_t left, uint32_t top, uint32_t right, uint32_t bottom ) throw()
{
	D3D11_RECT rect;
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;
	m_context->RSSetScissorRects( 1, &rect );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetNumberOfLights( uint32_t numLights ) throw()
{
	m_dirtyFlag |= m_renderStateEmulation.m_fragmentOpSettings.SetNumberOfLights( numLights );	
	return S_OK;
}

bool Tr2RenderContextAL::ApplyShadowRenderStates() throw()
{
	if( !m_secondaryDevice11 )
	{
		return false;
	}

	bool OK = true;

	if( m_vertexLayout )
	{
		if( !m_shaders[VERTEX_SHADER] || m_shaders[VERTEX_SHADER]->GetType() == INVALID_SHADER )
		{
			return false;
		}
		if( m_vertexLayout != m_lastSetVertexLayout || 
			m_shaders[VERTEX_SHADER]->GetInputDefinition().hash != m_lastSetVertexLayoutVSHash )
		{
			CR_RETURN_VAL(	m_vertexLayout->SetLayout( m_shaders[VERTEX_SHADER], *this )
						, false );

			m_lastSetVertexLayout = m_vertexLayout;
			m_lastSetVertexLayoutVSHash = m_shaders[VERTEX_SHADER]->GetInputDefinition().hash;
		}
	}
	else
	{
		m_lastSetVertexLayout = nullptr;
		m_lastSetVertexLayoutVSHash = (uint32_t)-1;
		m_context->IASetInputLayout( nullptr );
	}

	if( m_dirtyFlag )
	{
		if( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_FRAGMENTOP )
		{
			m_renderStateEmulation.m_fragmentOpSettings.UpdateContents( m_renderStateEmulation.m_alphaTestParameters );

			if( !m_fragmentOpBuffer.IsValid() )
			{
				auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
				CR_RETURN_VAL( 
					m_fragmentOpBuffer.Create(	sizeof( m_renderStateEmulation.m_fragmentOpSettings ), 
												USAGE_CPU_WRITE,
												nullptr,
												renderContext )
					, false );
			}

			{
				void * mapped = nullptr;
				CR_RETURN_VAL( m_fragmentOpBuffer.Lock( &mapped, *this ), false );

				if( !mapped )
				{
					return false;
				}
				memcpy( mapped, &m_renderStateEmulation.m_fragmentOpSettings,
						sizeof( m_renderStateEmulation.m_fragmentOpSettings ) );
				m_fragmentOpBuffer.Unlock( *this );
			}

			{
				SetConstants( m_fragmentOpBuffer, PIXEL_SHADER,  CONSTANT_BUFFER_FOR_FRAGMENT_OP_EMULATION );
				SetConstants( m_fragmentOpBuffer, VERTEX_SHADER, CONSTANT_BUFFER_FOR_FRAGMENT_OP_EMULATION );
			}
		}

		if( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_PATCH_PS )
		{
			if( !m_shaders[PIXEL_SHADER] )
			{
				return false;
			}
			if( m_renderStateEmulation.m_alphaTestParameters.m_alphaTestEnabled && 
				m_renderStateEmulation.m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS )
			{
				CR_RETURN_VAL( m_shaders[PIXEL_SHADER]->ApplyPatchedShader( *this ), false );
			}
			else
			{
				CR_RETURN_VAL( m_shaders[PIXEL_SHADER]->Apply( *this ), false );
			}
		}

		OK =	ApplyBlendState()		 &&
				ApplyDepthStencilState() &&
				ApplyRasterizerState();

		m_dirtyFlag = 0;
	}

	if( m_topology == TOP_TRIANGLES && ( m_topology != m_lastSetTopology || m_previouslyHadHullShader != m_hasHullShader ) )
	{
		if( m_hasHullShader )
		{
			m_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST );
		}
		else
		{
			m_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		}
		m_previouslyHadHullShader = m_hasHullShader;
		m_lastSetTopology = m_topology;
	}

	ApplyUavs();
	
	return OK;
}

void Tr2RenderContextAL::ApplyUavs() throw()
{
	if( m_psUavsDirtyBegin < m_psUavsDirtyEnd )
	{
		m_context->OMSetRenderTargetsAndUnorderedAccessViews( 
			D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
			nullptr,
			nullptr,
			m_psUavsDirtyBegin, 
			m_psUavsDirtyEnd - m_psUavsDirtyBegin,
			reinterpret_cast<ID3D11UnorderedAccessView**>( m_pixelShaderUavs + m_psUavsDirtyBegin ), 
			m_pixelShaderUavInitialCounts + m_psUavsDirtyBegin );
		m_psUavsDirtyBegin = sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] );
		m_psUavsDirtyEnd = 0;
	}
}

bool Tr2RenderContextAL::ApplyBlendState() throw()
{	
	if( !( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_BLEND ) )
	{
		return true;
	}

	CComPtr<ID3D11BlendState> blendState;
	CR_RETURN_VAL( m_renderStateEmulation.GetBlendState( blendState, m_secondaryDevice11 )
				, false );

	static const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_context->OMSetBlendState( blendState, blendFactor, 0xFFffFFff );
	return true;
}

bool Tr2RenderContextAL::ApplyDepthStencilState() throw()
{	
	if( !( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_DEPTHSTENCIL ) )
	{
		return true;
	}
	
	CComPtr<ID3D11DepthStencilState> depthStencilState;
	CR_RETURN_VAL( 
		m_renderStateEmulation.GetDepthStencilState( depthStencilState,
													 m_secondaryDevice11 )
		, false );

	m_context->OMSetDepthStencilState( depthStencilState, m_renderStateEmulation.m_currentStencilRef );
	return true;
}

bool Tr2RenderContextAL::ApplyRasterizerState() throw()
{	
	if( !( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_RASTERIZER ) )
	{
		return true;
	}

	m_renderStateEmulation.m_currentRasterizer.MultisampleEnable = true;	// has nothing to do with MSAAness of current RT, but has something to do with lines.
																			// "Recommended to always True" -- http://msdn.microsoft.com/en-us/library/windows/desktop/ff476198%28v=vs.85%29.aspx

	CComPtr<ID3D11RasterizerState> rasterizerState;
	CR_RETURN_VAL(
		m_renderStateEmulation.GetRasterizerState(	rasterizerState,
													m_secondaryDevice11 )
		, false );

	m_context->RSSetState( rasterizerState );
	return true;
}

ALResult Tr2RenderContextAL::SetResourceSet( const Tr2ResourceSetAL& resourceSet ) throw()
{
	if( !resourceSet.IsValid() )
	{
		return E_INVALIDARG;
	}

	if( m_currentResourceSet.m_resourceSet == resourceSet.m_resourceSet )
	{
		return S_OK; 
	}

	m_currentResourceSet = resourceSet;

	auto& rs = *resourceSet.m_resourceSet;

	if( rs.m_empty )
	{
		return S_OK;
	}

	decltype( &ID3D11DeviceContext::VSSetShaderResources ) setResources[] = {
		&ID3D11DeviceContext::VSSetShaderResources,
		&ID3D11DeviceContext::PSSetShaderResources,
		&ID3D11DeviceContext::CSSetShaderResources,
		&ID3D11DeviceContext::GSSetShaderResources,
		&ID3D11DeviceContext::HSSetShaderResources,
		&ID3D11DeviceContext::DSSetShaderResources,
	};

	decltype( &ID3D11DeviceContext::VSSetSamplers ) setSamplers[] = {
		&ID3D11DeviceContext::VSSetSamplers,
		&ID3D11DeviceContext::PSSetSamplers,
		&ID3D11DeviceContext::CSSetSamplers,
		&ID3D11DeviceContext::GSSetSamplers,
		&ID3D11DeviceContext::HSSetSamplers,
		&ID3D11DeviceContext::DSSetSamplers,
	};

	for( uint32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		auto& stage = rs.m_stages[i];
		if( stage.resourceCount && stage.resourceHash != m_resourceHashes[i] )
		{
			( m_context->*( setResources[i] ) )(
				stage.resourceOffset,
				stage.resourceCount,
				reinterpret_cast<ID3D11ShaderResourceView**>( stage.resources + stage.resourceOffset ) );
			m_resourceHashes[i] = stage.resourceHash;
		}
		if( stage.samplerCount && stage.samplerHash != m_samplerHashes[i] )
		{
			( m_context->*( setSamplers[i] ) )(
				stage.samplerOffset,
				stage.samplerCount,
				reinterpret_cast<ID3D11SamplerState**>( stage.samplers + stage.samplerOffset ) );
			m_samplerHashes[i] = stage.samplerHash;
		}
	}

	//if( rs.m_stages[VERTEX_SHADER].resourceCount && rs.m_stages[VERTEX_SHADER].resourceHash != m_resourceHashes[VERTEX_SHADER] )
	//{
	//	m_context->VSSetShaderResources(
	//		rs.m_stages[VERTEX_SHADER].resourceOffset,
	//		rs.m_stages[VERTEX_SHADER].resourceCount,
	//		reinterpret_cast<ID3D11ShaderResourceView**>( rs.m_stages[VERTEX_SHADER].resources + rs.m_stages[VERTEX_SHADER].resourceOffset ) );
	//	m_resourceHashes[VERTEX_SHADER] = rs.m_stages[VERTEX_SHADER].resourceHash;
	//}
	//if( rs.m_stages[PIXEL_SHADER].resourceCount && rs.m_stages[PIXEL_SHADER].resourceHash != m_resourceHashes[PIXEL_SHADER] )
	//{
	//	m_context->PSSetShaderResources(
	//		rs.m_stages[PIXEL_SHADER].resourceOffset,
	//		rs.m_stages[PIXEL_SHADER].resourceCount,
	//		reinterpret_cast<ID3D11ShaderResourceView**>( rs.m_stages[PIXEL_SHADER].resources + rs.m_stages[PIXEL_SHADER].resourceOffset ) );
	//	m_resourceHashes[PIXEL_SHADER] = rs.m_stages[PIXEL_SHADER].resourceHash;
	//}
	//if( rs.m_stages[COMPUTE_SHADER].resourceCount && rs.m_stages[COMPUTE_SHADER].resourceHash != m_resourceHashes[COMPUTE_SHADER] )
	//{
	//	m_context->CSSetShaderResources(
	//		rs.m_stages[COMPUTE_SHADER].resourceOffset,
	//		rs.m_stages[COMPUTE_SHADER].resourceCount,
	//		reinterpret_cast<ID3D11ShaderResourceView**>( rs.m_stages[COMPUTE_SHADER].resources + rs.m_stages[COMPUTE_SHADER].resourceOffset ) );
	//	m_resourceHashes[COMPUTE_SHADER] = rs.m_stages[COMPUTE_SHADER].resourceHash;
	//}
	//if( rs.m_stages[GEOMETRY_SHADER].resourceCount && rs.m_stages[GEOMETRY_SHADER].resourceHash != m_resourceHashes[GEOMETRY_SHADER] )
	//{
	//	m_context->GSSetShaderResources(
	//		rs.m_stages[GEOMETRY_SHADER].resourceOffset,
	//		rs.m_stages[GEOMETRY_SHADER].resourceCount,
	//		reinterpret_cast<ID3D11ShaderResourceView**>( rs.m_stages[GEOMETRY_SHADER].resources + rs.m_stages[GEOMETRY_SHADER].resourceOffset ) );
	//	m_resourceHashes[GEOMETRY_SHADER] = rs.m_stages[GEOMETRY_SHADER].resourceHash;
	//}
	//if( rs.m_stages[HULL_SHADER].resourceCount && rs.m_stages[HULL_SHADER].resourceHash != m_resourceHashes[HULL_SHADER] )
	//{
	//	m_context->HSSetShaderResources(
	//		rs.m_stages[HULL_SHADER].resourceOffset,
	//		rs.m_stages[HULL_SHADER].resourceCount,
	//		reinterpret_cast<ID3D11ShaderResourceView**>( rs.m_stages[HULL_SHADER].resources + rs.m_stages[HULL_SHADER].resourceOffset ) );
	//	m_resourceHashes[HULL_SHADER] = rs.m_stages[HULL_SHADER].resourceHash;
	//}
	//if( rs.m_stages[DOMAIN_SHADER].resourceCount && rs.m_stages[DOMAIN_SHADER].resourceHash != m_resourceHashes[DOMAIN_SHADER] )
	//{
	//	m_context->DSSetShaderResources(
	//		rs.m_stages[DOMAIN_SHADER].resourceOffset,
	//		rs.m_stages[DOMAIN_SHADER].resourceCount,
	//		reinterpret_cast<ID3D11ShaderResourceView**>( rs.m_stages[DOMAIN_SHADER].resources + rs.m_stages[DOMAIN_SHADER].resourceOffset ) );
	//	m_resourceHashes[DOMAIN_SHADER] = rs.m_stages[DOMAIN_SHADER].resourceHash;
	//}

	//if( rs.m_stages[VERTEX_SHADER].samplerCount && rs.m_stages[VERTEX_SHADER].samplerHash != m_samplerHashes[VERTEX_SHADER] )
	//{
	//	m_context->VSSetSamplers(
	//		rs.m_stages[VERTEX_SHADER].samplerOffset,
	//		rs.m_stages[VERTEX_SHADER].samplerCount,
	//		reinterpret_cast<ID3D11SamplerState**>( rs.m_stages[VERTEX_SHADER].samplers + rs.m_stages[VERTEX_SHADER].samplerOffset ) );
	//	m_samplerHashes[VERTEX_SHADER] = rs.m_stages[VERTEX_SHADER].samplerHash;
	//}
	//if( rs.m_stages[PIXEL_SHADER].samplerCount && rs.m_stages[PIXEL_SHADER].samplerHash != m_samplerHashes[PIXEL_SHADER] )
	//{
	//	m_context->PSSetSamplers(
	//		rs.m_stages[PIXEL_SHADER].samplerOffset,
	//		rs.m_stages[PIXEL_SHADER].samplerCount,
	//		reinterpret_cast<ID3D11SamplerState**>( rs.m_stages[PIXEL_SHADER].samplers + rs.m_stages[PIXEL_SHADER].samplerOffset ) );
	//	m_samplerHashes[PIXEL_SHADER] = rs.m_stages[PIXEL_SHADER].samplerHash;
	//}
	//if( rs.m_stages[COMPUTE_SHADER].samplerCount && rs.m_stages[COMPUTE_SHADER].samplerHash != m_samplerHashes[COMPUTE_SHADER] )
	//{
	//	m_context->CSSetSamplers(
	//		rs.m_stages[COMPUTE_SHADER].samplerOffset,
	//		rs.m_stages[COMPUTE_SHADER].samplerCount,
	//		reinterpret_cast<ID3D11SamplerState**>( rs.m_stages[COMPUTE_SHADER].samplers + rs.m_stages[COMPUTE_SHADER].samplerOffset ) );
	//	m_samplerHashes[COMPUTE_SHADER] = rs.m_stages[COMPUTE_SHADER].samplerHash;
	//}
	//if( rs.m_stages[GEOMETRY_SHADER].samplerCount && rs.m_stages[GEOMETRY_SHADER].samplerHash != m_samplerHashes[GEOMETRY_SHADER] )
	//{
	//	m_context->GSSetSamplers(
	//		rs.m_stages[GEOMETRY_SHADER].samplerOffset,
	//		rs.m_stages[GEOMETRY_SHADER].samplerCount,
	//		reinterpret_cast<ID3D11SamplerState**>( rs.m_stages[GEOMETRY_SHADER].samplers + rs.m_stages[GEOMETRY_SHADER].samplerOffset ) );
	//	m_samplerHashes[GEOMETRY_SHADER] = rs.m_stages[GEOMETRY_SHADER].samplerHash;
	//}
	//if( rs.m_stages[HULL_SHADER].samplerCount && rs.m_stages[HULL_SHADER].samplerHash != m_samplerHashes[HULL_SHADER] )
	//{
	//	m_context->HSSetSamplers(
	//		rs.m_stages[HULL_SHADER].samplerOffset,
	//		rs.m_stages[HULL_SHADER].samplerCount,
	//		reinterpret_cast<ID3D11SamplerState**>( rs.m_stages[HULL_SHADER].samplers + rs.m_stages[HULL_SHADER].samplerOffset ) );
	//	m_samplerHashes[HULL_SHADER] = rs.m_stages[HULL_SHADER].samplerHash;
	//}
	//if( rs.m_stages[DOMAIN_SHADER].samplerCount && rs.m_stages[DOMAIN_SHADER].samplerHash != m_samplerHashes[DOMAIN_SHADER] )
	//{
	//	m_context->DSSetSamplers(
	//		rs.m_stages[DOMAIN_SHADER].samplerOffset,
	//		rs.m_stages[DOMAIN_SHADER].samplerCount,
	//		reinterpret_cast<ID3D11SamplerState**>( rs.m_stages[DOMAIN_SHADER].samplers + rs.m_stages[DOMAIN_SHADER].samplerOffset ) );
	//	m_samplerHashes[DOMAIN_SHADER] = rs.m_stages[DOMAIN_SHADER].samplerHash;
	//}

	return S_OK;
}


ALResult Tr2RenderContextAL::SetUav(	
	Tr2RenderContextEnum::ShaderType inputType, 
	uint32_t slot, 
	Tr2TextureAL& texture,
	uint32_t mipLevel ) throw()
{
	ID3D11UnorderedAccessView* view = nullptr;

	if( texture.IsValid() )
	{
		if( mipLevel >= texture.m_texture->m_uav.size() )
		{
			return E_INVALIDARG;
		}
		view = texture.m_texture->m_uav[mipLevel];
		if( !view )
		{
			return E_INVALIDARG;
		}
	}

	switch( inputType )
	{
	case PIXEL_SHADER:
		if( slot >= sizeof( m_pixelShaderUavs ) / sizeof( m_pixelShaderUavs[0] ) )
		{
			return E_INVALIDARG;
		}
		m_pixelShaderUavs[slot] = view;
		if( m_psUavsDirtyBegin > slot )
		{
			m_psUavsDirtyBegin = slot;
		}
		if( m_psUavsDirtyEnd < slot + 1 )
		{
			m_psUavsDirtyEnd = slot + 1;
		}
		m_pixelShaderUavInitialCounts[slot] = (uint32_t)-1;
		break;
	case COMPUTE_SHADER:
		m_context->CSSetUnorderedAccessViews( slot, 1, &view, nullptr );
		break;
	default:
		return E_INVALIDARG;
	}
	
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::ClearUav( Tr2TextureAL& rt, uint32_t mipLevel, const float values[4] ) throw( )
{
	if( rt.m_texture->m_uav.size() <= mipLevel )
	{
		return E_INVALIDARG;
	}

	m_context->ClearUnorderedAccessViewFloat( rt.m_texture->m_uav[mipLevel], values );
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::ClearUav( Tr2TextureAL& rt, uint32_t mipLevel, const uint32_t values[4] ) throw( )
{
	if( rt.m_texture->m_uav.size() <= mipLevel )
	{
		return E_INVALIDARG;
	}

	m_context->ClearUnorderedAccessViewUint( rt.m_texture->m_uav[mipLevel], values );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport ) throw()
{
	static_assert( sizeof( viewport ) == sizeof( D3D11_VIEWPORT ), "viewport size mismatch" );
	m_context->RSSetViewports( 1, reinterpret_cast<const D3D11_VIEWPORT*>(&viewport) );
	float invWidth = 1.f / viewport.m_width;
	float invHeight = -1.f / viewport.m_height;
	if( invWidth != m_renderStateEmulation.m_fragmentOpSettings.m_renderTargetSize[0] ||
		invHeight != m_renderStateEmulation.m_fragmentOpSettings.m_renderTargetSize[1] )
	{
		m_renderStateEmulation.m_fragmentOpSettings.m_renderTargetSize[0] = invWidth;
		m_renderStateEmulation.m_fragmentOpSettings.m_renderTargetSize[1] = invHeight;
		m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_FRAGMENTOP;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport ) throw()
{
	static_assert( sizeof( viewport ) == sizeof( D3D11_VIEWPORT ), "viewport size mismatch" );
	uint32_t count = 1;
	m_context->RSGetViewports( &count, reinterpret_cast<D3D11_VIEWPORT*>(&viewport) );
	return S_OK;
}

ALResult Tr2RenderContextAL::PushRenderTarget( uint32_t slot ) throw()
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}

	m_stackRT[slot].push( m_boundRenderTarget[slot] );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopRenderTarget( uint32_t slot ) throw()
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	CCP_ASSERT( !m_stackRT[slot].empty() );
	if( m_stackRT[slot].empty() )
	{
		return E_FAIL;
	}

	m_boundRenderTarget[slot] = m_stackRT[slot].top();
	m_stackRT[slot].pop();
	return SetRtDsToDevice( slot );
}

ALResult Tr2RenderContextAL::PushDepthStencil() throw()
{
	m_stackDS.push( m_boundDepthStencil );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopDepthStencil() throw()
{
	CCP_ASSERT( !m_stackDS.empty() );
	if( m_stackDS.empty() )
	{
		return E_FAIL;
	}

	m_boundDepthStencil = m_stackDS.top();
	m_stackDS.pop();
	return SetRtDsToDevice( MAX_RENDER_TARGET );
}

ALResult Tr2RenderContextAL::GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot ) throw()
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}

	if( m_boundRenderTarget[slot].IsValid() )
	{
		width = m_boundRenderTarget[slot].GetDesc().GetWidth();
		height = m_boundRenderTarget[slot].GetDesc().GetHeight();
		return S_OK;
	}
	
	if( slot == 0 && m_secondarySwapChain )
	{		
		DXGI_SWAP_CHAIN_DESC desc;
		m_secondarySwapChain->GetDesc( &desc );
		width = desc.BufferDesc.Width;
		height = desc.BufferDesc.Height;
		return S_OK;
	}

	return E_FAIL;
}

bool Tr2RenderContextAL::IsBackBuffer( const Tr2TextureAL& rt ) const throw()
{
	return	m_secondaryDefaultBackBuffer == rt;
}

Tr2TextureAL& Tr2RenderContextAL::GetDefaultBackBuffer()
{
	return m_secondaryDefaultBackBuffer;
}

void Tr2RenderContextAL::ReleaseDeviceResources() throw()
{
	std::fill_n( m_shaders, int( SHADER_TYPE_COUNT ), nullptr );
}

void Tr2RenderContextAL::ResetCapturePlayback()
{
	Tr2RenderStateEmulation::s_blendStateCache.clear();
	Tr2RenderStateEmulation::s_depthStencilStateCache.clear();
	Tr2RenderStateEmulation::s_rasterizerCache.clear();

	memset( m_allRenderStates, 0xff, sizeof( m_allRenderStates ) );

	memset( &m_renderStateEmulation.m_currentBlend, 0, 
			sizeof( m_renderStateEmulation.m_currentBlend ) );
	m_renderStateEmulation.m_currentBlend.RenderTarget[0] = defaultBlend;

	m_renderStateEmulation.m_currentDepthStencil = defaultDepthStencil;
	m_renderStateEmulation.m_currentStencilRef = 0;
	m_renderStateEmulation.m_separateAlphaBlendEnabled = false;

	m_renderStateEmulation.m_currentRasterizer = defaultRasterizer;

	memset( &m_renderStateEmulation.m_fragmentOpSettings, 0, 
			sizeof( m_renderStateEmulation.m_fragmentOpSettings.m_alphaTestRef ) );

	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestRef = 0;
	m_renderStateEmulation.m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;
}

ALResult Tr2RenderContextAL::SetTopology( Tr2RenderContextEnum::Topology topology ) throw()
{
	using namespace Tr2RenderContextEnum;
	static D3D11_PRIMITIVE_TOPOLOGY lookup[TOP_MAX_TOPOLOGY] = 
	{
		D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,			// invalid

		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,

		D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,

		D3D11_PRIMITIVE_TOPOLOGY_POINTLIST		
	};

	if( topology == TOP_TRIANGLE_FAN )
	{
		CCP_AL_LOGWARN( "Using triangle fans on DX11 is no longer supported" );
	}

	if( topology >= TOP_MAX_TOPOLOGY )
	{
		return E_INVALIDARG;
	}

	m_topology = topology;
	// Defer setting TOP_TRIANGLES to ApplyShadowState because of hull shaders
	if( m_context )
	{
		if( m_topology != TOP_TRIANGLES && m_topology != m_lastSetTopology )
		{
			m_context->IASetPrimitiveTopology( lookup[ topology ] );
			m_lastSetTopology = m_topology;
		}
		return S_OK;
	}
	return E_FAIL;
}

// --------------------------------------------------------------------------------------
void Tr2RenderContextAL::AddGpuMarker( const char* marker )
{
	if( !m_aftermathContext )
	{
		return;
	}

	size_t length = strlen( marker ) + 1;
	if( length % 4 == 0)
	{
		GFSDK_Aftermath_SetEventMarker( reinterpret_cast<GFSDK_Aftermath_ContextHandle>( m_aftermathContext ), marker, unsigned( length ) );
		return;
	}

	size_t paddedLength = ( length / 4 + 1 ) * 4;
	std::unique_ptr<char[]> buffer( new char[paddedLength] );
	memcpy( buffer.get(), marker, length );
	GFSDK_Aftermath_SetEventMarker( reinterpret_cast<GFSDK_Aftermath_ContextHandle>( m_aftermathContext ), buffer.get(), unsigned( paddedLength ) );
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuStateMarker( RenderContextStatus& status, std::string& marker ) const
{
	status = CONTEXT_STATUS_INVALID;
	if( !m_aftermathContext )
	{
		return E_FAIL;
	}
	GFSDK_Aftermath_ContextData data;
	auto amResult = GFSDK_Aftermath_GetData( 1, reinterpret_cast<const GFSDK_Aftermath_ContextHandle*>( &m_aftermathContext ), &data );
	if( !GFSDK_Aftermath_SUCCEED( amResult ) )
	{
		return E_FAIL;
	}
	status = static_cast<RenderContextStatus>( data.status );
	marker = static_cast<const char*>( data.markerData );
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuPageFaultResource(
	Tr2RenderContextEnum::PixelFormat& format,
	uint64_t& size,
	uint32_t& width,
	uint32_t& height,
	uint32_t& depth,
	uint32_t& mips ) const
{
	CCP_UNUSED( mips );

	if( !m_aftermathContext )
	{
		return E_FAIL;
	}
	GFSDK_Aftermath_PageFaultInformation info;
	auto amResult = GFSDK_Aftermath_GetPageFaultInformation( &info );
	if( !GFSDK_Aftermath_SUCCEED( amResult ) )
	{
		return E_FAIL;
	}
	if( !info.bhasPageFaultOccured )
	{
		return E_FAIL;
	}
	format = static_cast<PixelFormat>( info.resourceDesc.format );
	size = info.resourceDesc.size;
	width = info.resourceDesc.width;
	height = info.resourceDesc.height;
	depth = info.resourceDesc.depth;
	return S_OK;
}

#endif	//DX11?
