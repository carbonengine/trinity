// Copyright © 2026 CCP ehf.

#pragma once

#include "IEveSpaceObjectChild.h"
#include "../../EveInstancedMeshManager.h"

BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2Effect );


BLUE_CLASS( EveChildInstancedMeshes ) :
	public IEveSpaceObjectChild,
	public EveEntity,
	public IBlueAsyncResNotifyTarget,
	public IEveShadowCaster,
	public IEveInstanceMeshProvider,
	public ITr2DebugRenderable,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveChildInstancedMeshes( IRoot* lockobj = NULL );
	~EveChildInstancedMeshes();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const override;
	void SetName( const char* name ) override;
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	void ChangeLOD( Tr2Lod lod ) override;
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

	struct MeshArea
	{
		Tr2EffectPtr effect = nullptr;
		TriBatchType batchType = TRIBATCHTYPE_OPAQUE;
		uint32_t areaIndex = 0;
		uint32_t areaCount = 1;
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
		const BlueSharedString& sofLocatorSetName );

	BluePy GetSofSourceLocator( uint32_t areaId ) const;
	uint32_t GetMeshCount() const;
	BluePy GetMeshInfo( uint32_t meshId ) const;
	BluePy GetAreaInfo( uint32_t meshId, uint32_t areaId ) const;
	BluePy GetMeshDisplay( uint32_t meshId ) const;
	BluePy SetMeshDisplay( uint32_t meshId, bool display );

private:
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

		bool display = true;
	};

	void ReleaseCachedData( BlueAsyncRes * p ) override;
	void RebuildCachedData( BlueAsyncRes * p ) override;
	void UnregisterFromMeshManager();

	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	BlueSharedString m_name;
	Matrix m_worldTransform = IdentityMatrix();
	EveSpacePerObjectData m_perObjectData;
	EveInstancedMeshManager::PerObjectDataHandle m_perObjectDataHandle;
	std::vector<Mesh> m_meshes;
	TriFrustum m_lastCameraFrustum;
	mutable Tr2ConstantBufferAL m_rtPerObjectData;
	bool m_allRegistered = false;

	// Has UpdateSynchronous/UpdateAsynchronous been called: until it was, the object can not be rendered
	bool m_hasUpdated = false;
};

TYPEDEF_BLUECLASS( EveChildInstancedMeshes );