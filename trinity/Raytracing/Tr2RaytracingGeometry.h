// Copyright © 2023 CCP ehf.

#pragma once

#include "include/ITr2GpuBuffer.h"
#include "Shader/Tr2Effect.h"
#include "../TbbStub.h"
#include "./Tr2RingBuffer.h"

BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2RaytracingGeometry );

struct TriGeometryResLodData;
class Tr2RtMesh;
class Tr2RtMeshArea;
struct Tr2MorphTargetAnimationData;

class Tr2RaytracingPipelineStateManager
{
public:
	Tr2RaytracingPipelineStateManager();

	bool AddLibrary( BlueSharedStringW& rayGenName, BlueSharedStringW& missName, Tr2Material* material, const BlueSharedString& techniqueName );
	void AddLibrary( BlueSharedStringW& rayGenName, BlueSharedStringW& missName, const Tr2EffectLibrary& library );
	BlueSharedStringW AddHitGroup( const Tr2EffectLibrary& library );
	Tr2RtPipelineStateAL GetPipelineState( Tr2RenderContext& renderContext );

private:
	std::wstring GetUniqueName();

	Tr2RtPipelineStateDescriptionAL m_pipelineDesc;
	Tr2RtPipelineStateAL m_pipelineState;
	std::unordered_map<uint32_t, BlueSharedStringW> m_hitGroups;
	std::unordered_map<uint32_t, std::pair<BlueSharedStringW, BlueSharedStringW>> m_libraries;
	uint32_t m_nextName;
	bool m_isDirty;
};


class Tr2RaytracingMesh
{
public:
	Tr2RaytracingMesh();

	void UpdateRtMesh( TriGeometryRes* geometry, uint32_t meshIndex, float screenSize );
	bool SetBoneTransforms( size_t count, const Float4x3* transforms, uint32_t offset );
	bool SetMorphAnimations( size_t count, const Tr2MorphTargetAnimationData* morphTargets, uint32_t morphTargetAnimationDataOffset );

	bool IsGood() const;
	bool IsGoodForArea( uint32_t area ) const;
	bool GetAndResetDirtyFlag();
	void MarkDirty();

	TriGeometryResLodData* GetCurrentLodData() const;
	TriGeometryResLodData* GetHighestLodData() const;
	uint32_t GetTransformOffset() const;
	const Tr2BufferAL* GetSkinnedVertexBuffer() const;
	const Tr2BufferAL& GetVertexBuffer() const;
	const Tr2BufferAL& GetIndexBuffer() const;

	void SetSkinnedVertices( const Tr2BufferAL& buffer, uint32_t offset );
	uint32_t GetSkinnedVertexOffset() const;

private:
	const Tr2BufferAL* m_skinnedVertices;
	TriGeometryResPtr m_geometry;
	uint32_t m_meshIndex;
	std::vector<float> m_transforms;
	uint32_t m_boneOffset;
	uint32_t m_skinnedVertexOffset;
	std::vector<uint8_t> m_morphAnimationDatas;
	uint32_t m_morphAnimationDataOffset;
	uint32_t m_morphAnimationDataCount;
	bool m_isDirty;
	float m_screenSize;
	int m_lodIndex;

	friend class Tr2RaytracingGeometry;
};

// Retained shader-table record last resolved for a mesh area; valid while the table generation, material and the
// stable addresses of the constant buffers its hit group binds are unchanged
struct Tr2RtRetainedRecordRef
{
	enum
	{
		BINDS_MATERIAL = 1,
		BINDS_PER_OBJECT = 2,
		BINDS_GEOMETRY = 4
	};

	uint32_t generation = 0;
	uint32_t index = 0;
	uint32_t techniqueIndex = 0;
	uint32_t boundMask = 0;
	const Tr2Shader* shader = nullptr;
	const Tr2Material* material = nullptr;
	const Tr2ConstantBufferAL* materialConstants = nullptr;
	const Tr2ConstantBufferAL* perObjectConstants = nullptr;
	const Tr2ConstantBufferAL* geometryConstants = nullptr;
	uint64_t materialAddress = 0;
	uint64_t perObjectAddress = 0;
	uint64_t geometryAddress = 0;
	bool isTransparent = false;
};

class Tr2RaytracingMeshArea
{
public:
	Tr2RaytracingMeshArea( uint32_t index );
	const Tr2RtBottomLevelAccelerationStructureAL& BuildBlas( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext );
	const TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL* GetBuiltStaticBlas( const Tr2RaytracingMesh& mesh ) const;
	const Tr2ConstantBufferAL* GetGeometryConstants( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext ) const;
	bool FindRetainedRecord( const Tr2Material* material, const Tr2ConstantBufferAL* perObjectData, const Tr2ConstantBufferAL* geometryConstants, uint32_t generation, uint32_t& index, bool& isTransparent ) const;
	uint32_t GetAreaIndex()
	{
		return m_areaIndex;
	}
	void MarkBlasOutdated()
	{
		m_blasOutdated = true;
	}
	bool IsBlasOutdated() const
	{
		return m_blasOutdated || !m_blas.IsValid();
	}

private:
	uint32_t m_areaIndex;

	Tr2RtBottomLevelAccelerationStructureAL m_blas;
	bool m_blasOutdated;
	mutable const TriGeometryResLodData* m_geometryConstantsLod = nullptr;
	mutable const Tr2ConstantBufferAL* m_geometryConstants = nullptr;
	mutable uint32_t m_geometryConstantsKey[4] = {};
	Tr2RtRetainedRecordRef m_retainedRecord;

	friend class Tr2RaytracingGeometry;
};

BLUE_CLASS( Tr2RaytracingGeometry ) :
	public ITr2GpuBuffer
{
public:
	EXPOSE_TO_BLUE();

	Tr2RaytracingGeometry();

	Tr2BufferAL* GetGpuBuffer( unsigned index ) override;
	void BeginSceneUpdate();
	void EndSceneUpdate( Tr2RenderContext & renderContext, int32_t numRaycasters, Tr2RtShaderTableDescriptionAL** shaderTableDescs, Tr2RaytracingPipelineStateManager** pipelineManagers );
	void AddGeometry( Tr2RaytracingMesh & mesh, Tr2RaytracingMeshArea & area, Tr2Material * material, const Tr2ConstantBufferAL* perObjectData, const Tr2ConstantBufferAL* vertexBufferData, const Matrix& worldTransform, uint32_t bakedMorphOffset = std::numeric_limits<uint32_t>::max() );
	void AddGeometry( Tr2RaytracingMesh & mesh, Tr2RaytracingMeshArea & area, Tr2Material * material, const Tr2ConstantBufferAL* perObjectData, const Tr2ConstantBufferAL* vertexBufferData, const Float4x3* worldTransforms, size_t instanceCount, uint32_t bakedMorphOffset = std::numeric_limits<uint32_t>::max() );
	void AddBindlessResources( const Tr2MeshAreaVector& areas, const Tr2RaytracingMesh& rtMesh );
	bool HasGeometry() const;

	void ReleaseResources( TriStorage s );

	Tr2RtTopLevelAccelerationStructureAL GetTLAS() const;
	const Tr2BindlessResourcesAL& GetBindlessResources() const;

	const BlueSharedString m_rtShadowTechniqueName = BlueSharedString( "RtShadow" );

private:
	struct VtxOffsets
	{
		uint32_t positionOffset;
		uint32_t boneOffset;
		uint32_t boneWeightsOffset;
	};

	struct GeometryData
	{
		Tr2RaytracingMesh* mesh;
		Tr2RaytracingMeshArea* area;
		const Tr2Material* material;
		const Tr2ConstantBufferAL* perObjectData;
		const Tr2ConstantBufferAL* vertexBufferData;
		Matrix worldTransform;
		const Float4x3* worldTransforms = nullptr;
		uint32_t instanceCount = 1;
		uint32_t materialIndex;
		bool isTransparent;
		uint32_t bakedMorphOffset;
		Tr2RtInstanceAL instance;
		bool instanceTransformPrepared;
	};

	const BlueSharedString m_inVertexBufferTechniqueName = BlueSharedString( "InVB" );
	const BlueSharedString m_outVertexBufferTechniqueName = BlueSharedString( "OutVB" );
	static const uint32_t INVALID_MATERIAL = 0xffffffff;

	struct RetainedRecordKey
	{
		uint32_t libraryHandle;
		uint64_t address[3];

		bool operator==( const RetainedRecordKey& other ) const
		{
			return libraryHandle == other.libraryHandle && address[0] == other.address[0] && address[1] == other.address[1] && address[2] == other.address[2];
		}
	};
	struct RetainedRecordKeyHash
	{
		size_t operator()( const RetainedRecordKey& key ) const
		{
			uint64_t h = key.libraryHandle * 0x9E3779B97F4A7C15ull;
			for( auto address : key.address )
			{
				h ^= ( address + 0x9E3779B97F4A7C15ull ) * 0xff51afd7ed558ccdull;
				h ^= h >> 29;
			}
			return size_t( h );
		}
	};
	struct TransientRecord
	{
		GeometryData* geometry;
		const std::vector<BlueSharedStringW>* hitGroupNames;
		Tr2RtLocalMaterialDescriptionAL material;
	};

	void PrepareShaderTableDescription( Tr2RenderContext & renderContext, int32_t numRaycasters, Tr2RtShaderTableDescriptionAL** shaderTableDescs, Tr2RaytracingPipelineStateManager** pipelineManagers );
	void PrepareRetainedShaderTableDescription( Tr2RenderContext & renderContext, int32_t numRaycasters, Tr2RtShaderTableDescriptionAL** shaderTableDescs, Tr2RaytracingPipelineStateManager** pipelineManagers );
	void ResetRetainedRecords();
	void TransformMeshes( Tr2RenderContext & renderContext );
	void BuildAccelerationStructures( Tr2RenderContext & renderContext );
	void NoteDeformedGeometry( Tr2RaytracingMesh & mesh, Tr2RaytracingMeshArea & area );

	VtxOffsets FindOffsets( unsigned declHandle );

	std::vector<GeometryData> m_geometryData;
	Tr2EnumerableThreadSpecific<std::vector<GeometryData>> m_threadLocalGeometryData;
	std::unordered_map<RetainedRecordKey, uint32_t, RetainedRecordKeyHash> m_retainedRecords;
	std::vector<uint64_t> m_retainedLastUsed;
	std::vector<TransientRecord> m_transientRecords;
	uint32_t m_retainedCount = 0;
	uint32_t m_recordGeneration = 1;
	int32_t m_retainedRaycasters = 0;
	uint64_t m_recordFrame = 0;
	std::atomic<bool> m_hasDeformedGeometry{ false };
	std::vector<Tr2RtInstanceAL> m_instances;
	Tr2RtTopLevelAccelerationStructureAL m_tlas;

	Tr2EffectPtr m_skinVerticesEffect;
	Tr2ConstantBufferAL m_skinVerticesData;
	std::unordered_map<unsigned, VtxOffsets> m_offsets;

	Tr2BufferAL m_skinnedVertices;

	Tr2BindlessResourcesAL m_usedResources;
	Tr2EnumerableThreadSpecific<Tr2BindlessResourcesAL> m_threadLocalUsedResources;
};

TYPEDEF_BLUECLASS( Tr2RaytracingGeometry );
