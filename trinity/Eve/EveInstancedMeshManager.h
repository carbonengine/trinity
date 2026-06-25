// Copyright © 2026 CCP ehf.

#pragma once

#include "SpaceObject/EveSpaceObject2.h"
#include "Eve/EvePicking.h"

BLUE_DECLARE( Tr2GpuStructuredBuffer );

class EveInstancedMeshManager
{
public:
	struct InstanceFlags
	{
		void AddBatchType( TriBatchType type );
		void SetCastsShadow( bool castsShadow );
		bool GetCastsShadow() const;
		void SetRenderInReflections( bool renderInReflections );
		bool MatchesFilter( const InstanceFlags& filter ) const;
		bool operator==( const InstanceFlags& other ) const;
		bool operator!=( const InstanceFlags& other ) const;

		static const uint32_t CASTS_SHADOW = 1 << 30;
		static const uint32_t RENDER_IN_REFLECTION = 1 << 31;
		uint32_t m_flags = 0;
	};

	struct DynamicPerInstanceData
	{
		Vector4 worldTransform[3];
		Vector4 prevWorldTransform[3];
		uint32_t sphereIndex = 0;
	};

	struct StaticPerInstanceData
	{
		Vector4 worldTransform[3];
		uint32_t sphereIndex = 0;
	};

	template <typename T>
	struct DataHandle
	{
		DataHandle() = default;
		DataHandle( const DataHandle& ) = delete;
		DataHandle& operator=( const DataHandle& ) = delete;
		DataHandle( DataHandle&& other ) noexcept
		{
			if( owner )
			{
				owner->ReplaceHandle( this, &other );
			}
			owner = other.owner;
			index = other.index;
			other.owner = nullptr;
			other.index = InvalidIndex;
		}

		operator bool() const
		{
			return index != InvalidIndex;
		}


		static const uint32_t InvalidIndex = 0xFFFFFFFF;
		EveInstancedMeshManager* owner = nullptr;
		uint32_t index = InvalidIndex;
	};

	using PerObjectDataHandle = DataHandle<struct PerObjectDataTag>;
	using BoundingSphereHandle = DataHandle<struct BoundingSphereTag>;
	using MeshGroupHandle = DataHandle<struct MeshGroupTag>;

	void CollectMeshes( EveComponentRegistry& registry );
	size_t GetBatches( const TriFrustum& frustum, float invLodFactor, const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	size_t GetShadowBatches( const TriFrustum& frustum, const IEveShadowFrustum& shadowFrustum, float invLodFactor, const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	void GetPickingBatches( EvePendingPickingReadback& readback, const TriFrustum& viewFrustum, const TriFrustum& pickingFrustum, float invLodFactor, uint32_t objectIdOffset, const std::vector<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches );
	std::pair<IRootPtr, uint32_t> GetPickedObject( uint32_t objectId, uint32_t areaId );
	void ReportUsedScreenSizes() const;

	void AddPerObjectData( PerObjectDataHandle& handle, const EveSpacePerObjectData* data );
	void RemovePerObjectData( PerObjectDataHandle& handle );
	void ReplaceHandle( PerObjectDataHandle* oldHandle, PerObjectDataHandle* newHandle );

	void AddBoundingSphereGroup( BoundingSphereHandle& handle, const CcpMath::Sphere& bounds, const InstanceFlags& flags, const CcpMath::Sphere* boundingSpheres, uint32_t count );
	void RemoveBoundingSphereGroup( BoundingSphereHandle& handle );
	void SetSphereGroupBounds( const BoundingSphereHandle& handle, const CcpMath::Sphere& bounds, const InstanceFlags& flags );
	void ReplaceHandle( BoundingSphereHandle* oldHandle, BoundingSphereHandle* newHandle );

	void AddMeshGroup(
		MeshGroupHandle& handle,
		TriGeometryRes* geometry,
		unsigned combinedVertexDeclaration,
		TriBatchType batchType,
		uint32_t meshIndex,
		uint32_t areaIndex,
		uint32_t areaCount,
		Tr2Effect* material,
		uint64_t materialHash,
		const PerObjectDataHandle& perObjectDataHandle,
		const BoundingSphereHandle& sphereHandle,
		const DynamicPerInstanceData* perInstanceData,
		uint32_t count,
		IRoot* pickingOwner,
		uint32_t pickingOwnerIndex );
	void AddMeshGroup(
		MeshGroupHandle& handle,
		TriGeometryRes* geometry,
		unsigned combinedVertexDeclaration,
		TriBatchType batchType,
		uint32_t meshIndex,
		uint32_t areaIndex,
		uint32_t areaCount,
		Tr2Effect* material,
		uint64_t materialHash,
		const PerObjectDataHandle& perObjectDataHandle,
		const BoundingSphereHandle& sphereHandle,
		const StaticPerInstanceData* perInstanceData,
		uint32_t count,
		IRoot* pickingOwner,
		uint32_t pickingOwnerIndex );
	void RemoveMeshGroup( MeshGroupHandle& handle );
	void ReplaceHandle( MeshGroupHandle* oldHandle, MeshGroupHandle* newHandle );

private:
	struct StaticPerInstanceBufferElement
	{
		Vector4 worldTransform[3];
		uint32_t perObjectDataIndex = 0;
	};

	struct DynamicPerInstanceBufferElement
	{
		Vector4 worldTransform[3];
		Vector4 prevWorldTransform[3];
		uint32_t perObjectDataIndex = 0;
	};

	struct MeshKey
	{
		TriGeometryRes* geometry = nullptr;
		uint64_t materialHash = 0;
		uint32_t combinedVertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
		TriBatchType batchType = TRIBATCHTYPE_OPAQUE;
		uint32_t meshIndex = 0;
		uint32_t areaIndex = 0;
		uint32_t areaCount = 1;
		bool isDynamic = false;

		bool operator==( const MeshKey& other ) const
		{
			return geometry == other.geometry &&
				batchType == other.batchType &&
				meshIndex == other.meshIndex &&
				areaIndex == other.areaIndex &&
				areaCount == other.areaCount &&
				materialHash == other.materialHash;
		}
	};

	struct MeshGroup
	{
		MeshGroupHandle* handle = nullptr;
		union
		{
			const StaticPerInstanceData* staticInstances = nullptr;
			const DynamicPerInstanceData* dynamicInstances;
		};
		uint32_t count = 0;
		uint32_t sphereGroupIndex = 0;
		uint32_t perObjectDataIndex = 0;
		uint32_t ownerIndex = 0;
		IRootPtr owner; // Used for picking
		uint32_t pickingObjectId = 0;
	};

	class InstanceBuffer
	{
	public:
		struct Allocation
		{
			Tr2BufferAL* buffer = nullptr;
			uint8_t* data = nullptr;
			uint32_t offset = 0;
		};

		Allocation Allocate( uint32_t count );
		std::optional<uint32_t> GetUnusedRegion( uint32_t minSize );
		void DoneCopying();
		void TrimRegions( uint64_t renderedFrame );
		uint32_t GetSize() const;

	private:
		struct Region
		{
			uint32_t offset = 0;
			uint32_t length = 0;
			uint64_t recordedFrame = 0;
		};

		std::unique_ptr<Tr2BufferAL> buffer;
		std::vector<Region> regions;
		std::vector<std::unique_ptr<Tr2BufferAL>> retiredBuffers;
		std::vector<Tr2BufferAL*> mappedRetiredBuffers;
		uint8_t* mappedData = nullptr;
	};

	struct MeshData
	{
		std::vector<MeshGroup> meshGroups;

		Tr2Effect* material = nullptr;
		float radius = 0.0f;
		float maxScreenSize = 0.0f;
		uint32_t totalVisibleInstances = 0;
		InstanceBuffer::Allocation currentAllocation;

		std::vector<std::vector<std::pair<const void*, uint32_t>>> lodIndices;
		std::vector<float> screenSizeThresholds;
	};

	struct KeyHasher
	{
		std::size_t operator()( const MeshKey& k ) const
		{
			const uint8_t* ptr = reinterpret_cast<const uint8_t*>( &k );
			return CcpHashFNV1( ptr, sizeof( MeshKey ), unsigned( k.materialHash ) );
		}
	};

	struct SphereGroup
	{
		CcpMath::Sphere bounds = {};
		InstanceFlags flags;
		uint32_t count = 0;
		const CcpMath::Sphere* boundingSpheres = nullptr;
		BoundingSphereHandle* handle = nullptr;
		std::vector<float> screenSizes;
		TriFrustumTestResult lastTestResult = TriFrustumTestResult::Outside;
	};

	void PerformFrustumCulling( const TriFrustum& cameraFrustum, float invLodFactor, InstanceFlags filter );
	void PerformFrustumCulling( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, float invLodFactor, InstanceFlags filter );
	void PerformFrustumCulling( const TriFrustum& cameraFrustum, const TriFrustum& pickingFrustum, float invLodFactor, InstanceFlags filter );
	std::pair<uint32_t, float> BinVisibleInstances( const MeshKey& mesh, MeshData& meshInfo );
	std::pair<uint32_t, float> BinVisibleInstances( const MeshKey& mesh, MeshData& meshInfo, MeshGroup& group );
	void BinVisibleInstances( const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches );

	size_t GetBatches( const std::initializer_list<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches );
	void GetPickingBatches( EvePendingPickingReadback& readback, uint32_t objectIdOffset, const std::vector<std::pair<TriBatchType, ITriRenderBatchAccumulator&>>& batches );
	static uint32_t GetMeshLod( const MeshData& meshInfo, float screenSize );
	void UploadLodData( const MeshKey& mesh, MeshData& meshInfo, uint32_t lod, InstanceBuffer::Allocation& allocation );
	InstanceBuffer::Allocation AllocateInstanceData( uint32_t count, bool isDynamic );


	std::unordered_map<MeshKey, MeshData, KeyHasher> m_meshInstances;
	std::vector<std::pair<const EveSpacePerObjectData*, PerObjectDataHandle*>> m_perObjectData;
	std::vector<SphereGroup> m_sphereGroups;
	Tr2GpuStructuredBufferPtr m_perObjectDataBuffer;
	InstanceBuffer m_staticInstanceBuffer;
	InstanceBuffer m_dynamicInstanceBuffer;

	size_t m_instanceCount = 0;
};



struct IEveInstanceMeshProvider
{
	virtual void AddMeshesToManager( EveInstancedMeshManager& manager ) = 0;
};

REGISTER_COMPONENT_TYPE( "InstancedMeshProvider", IEveInstanceMeshProvider );
