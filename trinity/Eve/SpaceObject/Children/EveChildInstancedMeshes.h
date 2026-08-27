// Copyright © 2026 CCP ehf.

#pragma once

#include "EveSpaceObjectChild.h"
#include "../../EveInstancedMeshManager.h"
#include "ITr2Renderable.h"
#include "Tr2PersistentPerObjectData.h"
#include "../Attachments/EveMeshOverlayEffect.h"

BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2Effect );


BLUE_CLASS( EveChildInstancedMeshes ) :
	public EveSpaceObjectChild,
	public EveEntity,
	public IBlueAsyncResNotifyTarget,
	public IEveShadowCaster,
	public IEveInstanceMeshProvider,
	public ITr2DebugRenderable,
	public ITr2Renderable,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveChildInstancedMeshes( IRoot* lockobj = NULL );
	~EveChildInstancedMeshes();

	/////////////////////////////////////////////////////////////////////////////////////
	// EveSpaceObjectChild
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;


	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	bool IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const override;
	void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) override;
	Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;
	void PushRtGeometry( Tr2RaytracingManager& ) const override;


	//////////////////////////////////////////////////////////////////////////////////////
	// IEveInstanceMeshProvider
	void AddMeshesToManager( EveInstancedMeshManager & manager ) override;


	//////////////////////////////////////////////////////////////////////////////////////
	// IEveInstanceMeshProvider
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	bool HasTransparentBatches() override;
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;

	struct MeshArea
	{
		Tr2EffectPtr effect = nullptr;
		TriBatchType batchType = TRIBATCHTYPE_OPAQUE;
		uint32_t areaIndex = 0;
		uint32_t areaCount = 1;
		bool alphaCutout = false;
		bool reversed = false;
		uint64_t effectHash = 0;
		EveInstancedMeshManager::MeshGroupHandle meshGroupHandle;
	};

	void AddMesh(
		const char* geometryPath,
		bool castsShadow,
		EntityComponents::ReflectionMode reflectionMode,
		uint32_t meshIndex,
		const MeshArea* areas,
		size_t areaCount,
		const Matrix* instanceTransforms,
		size_t count,
		const BlueSharedString& sofHullName,
		const BlueSharedString& sofLocatorSetName,
		EveSpaceObjectChild::PartTag partTag = EveSpaceObjectChild::NO_PART_TAG );

	void RemoveInstancesByPartTag( EveSpaceObjectChild::PartTag partTag );

	BluePy GetSofSourceLocator( uint32_t areaId ) const;
	uint32_t GetMeshCount() const;
	BluePy GetMeshInfo( uint32_t meshId ) const;
	BluePy GetAreaInfo( uint32_t meshId, uint32_t areaId ) const;
	BluePy GetMeshDisplay( uint32_t meshId ) const;
	BluePy SetMeshDisplay( uint32_t meshId, bool display );
	BluePy GetMeshInheritOverlayEffects( uint32_t meshId ) const;
	BluePy SetMeshInheritOverlayEffects( uint32_t meshId, bool inherit );

	BluePy AddMeshOverlayEffect( uint32_t meshId, EveMeshOverlayEffect* overlayEffect );
	BluePy RemoveMeshOverlayEffect( uint32_t meshId, EveMeshOverlayEffect* overlayEffect );
	BluePy ClearMeshOverlayEffects( uint32_t meshId );
	BluePy GetMeshOverlayEffectCount( uint32_t meshId ) const;

	void CollectOwnedGeometry( TriBatchType type, const Matrix& parentTransform, std::vector<EveChildGeometry>& out, std::vector<EveChildGeometryArea>& areaPool ) const override;

private:
	// per-instance constant buffers for overlay draws
	struct OverlayInstancePod
	{
		EveSpaceObjectVSData vsData = {};
		EveSpaceObjectPSData psData = {};
		Tr2PersistentPerObjectData<OverlayInstancePod> vsBuffer;
		Tr2PersistentPerObjectData<OverlayInstancePod> psBuffer;
		Tr2PerObjectData* framePod = nullptr; // valid only within the current frame's batch gathering

		uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
		void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data );
	};

	struct Mesh
	{
		std::string geometryPath;
		TriGeometryResPtr geometry;
		std::vector<MeshArea> areas;
		unsigned combinedVertexDeclaration = unsigned( Tr2EffectStateManager::UNINITIALIZED_DECLARATION );
		uint32_t meshIndex = 0;

		CcpMath::Sphere worldBoundingSphere;
		EveInstancedMeshManager::InstanceFlags flags;
		EntityComponents::ReflectionMode reflectionMode = EntityComponents::REFLECT_NEVER;

		std::vector<EveInstancedMeshManager::StaticPerInstanceData> instances;
		std::vector<CcpMath::Sphere> instanceSpheres;
		std::vector<uint32_t> partTags;

		EveInstancedMeshManager::BoundingSphereHandle sphereHandle;

		BlueSharedString sofHullName;
		BlueSharedString sofLocatorSetName;

		struct RayTracingArea
		{
			Tr2RaytracingMeshArea* rtMeshArea = nullptr;
			uint32_t areaIndex = 0;
		};
		struct RayTracingMesh
		{
			Tr2RaytracingMesh* rtMesh = nullptr;
			std::vector<RayTracingArea> rtMeshAreas;

			mutable std::vector<Float4x3> instanceWorldTransforms;
			float maxScreenSize = 0.0f;
		};
		std::vector<RayTracingMesh> rtMeshes;

		std::unique_ptr<std::vector<OverlayInstancePod>> overlayPods;
		std::vector<TriRenderBatchAreaBlock> overlayAreaBlocks[EveMeshOverlayEffect::TYPE_COUNT];
		bool overlayAreaBlocksBuilt = false;
		std::vector<EveMeshOverlayEffectPtr> ownOverlayEffects;

		bool display = true;
		bool inheritOverlayEffects = true;
	};

	void ReleaseCachedData( BlueAsyncRes * p ) override;
	void RebuildCachedData( BlueAsyncRes * p ) override;
	void UnregisterFromMeshManager();

	void UpdateOverlayInstanceData( const EveSpaceObjectVSData& parentVsData, const EveSpaceObjectPSData& parentPsData );
	static void RebuildOverlayAreaBlocks( Mesh & mesh );
	bool HasAnyOwnOverlayEffects() const;
	bool AnyMeshInheritsOverlayEffects() const;
	bool MeshHasActiveOverlayEffects( const Mesh& mesh ) const;

	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	Matrix m_worldTransform = IdentityMatrix();
	EveSpacePerObjectData m_perObjectData;
	// m_perObjectData with the clip sphere neutralized, for meshes that opt out of the parent's overlays
	EveSpacePerObjectData m_perObjectDataNoClip;
	const PEveMeshOverlayEffectVector* m_parentOverlayEffects = nullptr;
	EveInstancedMeshManager::PerObjectDataHandle m_perObjectDataHandle;
	EveInstancedMeshManager::PerObjectDataHandle m_perObjectDataNoClipHandle;
	std::vector<Mesh> m_meshes;
	TriFrustum m_lastCameraFrustum;
	float m_lastInvLodFactor = 1.0f;
	mutable Tr2ConstantBufferAL m_rtPerObjectData;
	bool m_allRegistered = false;

	// Has UpdateSynchronous/UpdateAsynchronous been called: until it was, the object can not be rendered
	bool m_hasUpdated = false;
};

TYPEDEF_BLUECLASS( EveChildInstancedMeshes );