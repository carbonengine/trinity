////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


namespace TrinityALImpl
{
	D3D12_RESOURCE_BARRIER Transition(
		ID3D12Resource* pResource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE );

	D3D12_RESOURCE_BARRIER AliasBarrier(
		ID3D12Resource* before,
		ID3D12Resource* after );

	D3D12_HEAP_PROPERTIES HeapDesc( D3D12_HEAP_TYPE type );

	void MemcpySubresource(
		_In_ const D3D12_MEMCPY_DEST* pDest,
		_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
		SIZE_T RowSizeInBytes,
		UINT NumRows,
		UINT NumSlices );

	UINT64 UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		_In_range_( 0, D3D12_REQ_SUBRESOURCES ) UINT FirstSubresource,
		_In_range_( 0, D3D12_REQ_SUBRESOURCES - FirstSubresource ) UINT NumSubresources,
		UINT64 RequiredSize,
		_In_reads_( NumSubresources ) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		_In_reads_( NumSubresources ) const UINT* pNumRows,
		_In_reads_( NumSubresources ) const UINT64* pRowSizesInBytes,
		_In_reads_( NumSubresources ) const D3D12_SUBRESOURCE_DATA* pSrcData );

	UINT64 UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		UINT64 IntermediateOffset,
		_In_range_( 0, D3D12_REQ_SUBRESOURCES ) UINT FirstSubresource,
		_In_range_( 0, D3D12_REQ_SUBRESOURCES - FirstSubresource ) UINT NumSubresources,
		_In_reads_( NumSubresources ) const D3D12_SUBRESOURCE_DATA* pSrcData );

	//------------------------------------------------------------------------------------------------
	// Stack-allocating UpdateSubresources implementation
	template <UINT MaxSubresources>
	inline UINT64 UpdateSubresources(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		UINT64 IntermediateOffset,
		_In_range_( 0, MaxSubresources ) UINT FirstSubresource,
		_In_range_( 1, MaxSubresources - FirstSubresource ) UINT NumSubresources,
		_In_reads_( NumSubresources ) const D3D12_SUBRESOURCE_DATA* pSrcData )
	{
		UINT64 RequiredSize = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
		UINT NumRows[MaxSubresources];
		UINT64 RowSizesInBytes[MaxSubresources];

		D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
		ID3D12Device* pDevice;
		pDestinationResource->GetDevice( __uuidof( ID3D12Device ), reinterpret_cast<void**>( &pDevice ) );
		pDevice->GetCopyableFootprints( &Desc, FirstSubresource, NumSubresources, IntermediateOffset, Layouts, NumRows, RowSizesInBytes, &RequiredSize );
		pDevice->Release();

		return UpdateSubresources( pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, Layouts, NumRows, RowSizesInBytes, pSrcData );
	}

	UINT64 GetRequiredIntermediateSize(
		_In_ ID3D12Resource* pDestinationResource,
		_In_range_( 0, D3D12_REQ_SUBRESOURCES ) UINT FirstSubresource,
		_In_range_( 0, D3D12_REQ_SUBRESOURCES - FirstSubresource ) UINT NumSubresources );

	D3D12_RESOURCE_DESC BufferDesc( uint64_t size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE );


	class GenerateMipsResources
	{
	public:
		GenerateMipsResources( _In_ ID3D12Device* device );

		CComPtr<ID3D12RootSignature> rootSignature;
		CComPtr<ID3D12PipelineState> generateMipsPSO;
		CComPtr<ID3D12PipelineState> generateMipsArrayPSO;

		enum RootParameterIndex
		{
			Constants,
			SourceTexture,
			TargetTexture,
			RootParameterCount
		};

#pragma pack(push, 4)
		struct ConstantData
		{
			float InvOutTexelSizeX;
			float InvOutTexelSizeY;
			uint32_t SrcMipIndex;
		};
#pragma pack(pop)

		static const uint32_t Num32BitConstants = static_cast<uint32_t>( sizeof( ConstantData ) / sizeof( uint32_t ) );
		static const uint32_t ThreadGroupSize = 8;

	private:
		static CComPtr<ID3D12RootSignature> CreateGenMipsRootSignature(
			_In_ ID3D12Device* device );

		static CComPtr<ID3D12PipelineState> CreateGenMipsPipelineState(
			_In_ ID3D12Device* device,
			_In_ ID3D12RootSignature* rootSignature,
			_In_reads_( bytecodeSize ) const uint8_t* bytecode,
			_In_ size_t bytecodeSize );
	};

	void SetDebugName( ID3D12DeviceChild* resource, const char* name );
}

#endif