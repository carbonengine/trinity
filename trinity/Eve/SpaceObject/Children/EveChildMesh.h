// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveChildMesh_H
#define EveChildMesh_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Eve/SpaceObject/Attachments/IEveSpaceObjectDecalOwner.h"
#include "Lights/Tr2Light.h"
#include "Lights/ITr2LightOwner.h"
#include "Eve/SpaceObject/Attachments/Sets/IEveSpaceObjectAttachment.h"
#include "Eve/SpaceObject/Attachments/Sets/IEveSpaceObjectAttachmentOwner.h"
#include "ITr2Renderable.h"
#include "ITr2MeshMorph.h"
#include "Resources/Tr2LodResource.h"
#include "TransformModifiers/IEveChildTransformModifier.h"
#include "Tr2DebugRenderer.h"
#include "Raytracing/Tr2RaytracingManager.h"
#include "../Tr2RingBuffer.h"
#include "Tr2SuballocatedBuffer.h"

class EveUpdateContext;
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( Tr2GpuBufferWrapper );

extern Tr2SuballocatedBuffer g_bakedMorphTargetBuffer;

struct MergeMorphsConstantBuffer
{
	uint32_t morphTargetVertexDataOffset;
	uint32_t morphTargetAnimationDataOffset;
	uint32_t activeMorphTargetsCount;
	uint32_t bakedMorphTargetVertexDataOffset;
	uint32_t vertexDataOffset;
	uint32_t vertexDataStride;
	uint32_t vertexDataPositionOffset;
	uint32_t vertexDataTangentOffset;
	uint32_t vertexCount;
	uint32_t padding1;
	uint32_t padding2;
	uint32_t padding3;
};

enum class MorphTargetAnimationFilter : uint8_t
{
	RUNTIME_EVALUATED,
	BAKED,
	ALL
};

BLUE_CLASS( EveChildMesh ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize,
	public ITr2DebugRenderable,
	public IListNotify,
	public ITr2GrannyAnimationOwner,
	public EveEntity,
	public INotify,
	public IEveSpaceObjectDecalOwner,
	public IEveSpaceObjectAttachmentOwner,
	public ITr2LightOwner,
	public IEveShadowCaster,
	public Tr2DeviceResource,
	public ITr2MeshMorph,
	public ITr2Pickable
{
public:
	EXPOSE_TO_BLUE();

	EveChildMesh( IRoot* lockobj = NULL );
	~EveChildMesh();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void ChangeLOD( Tr2Lod lod ) override;
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	bool IsAlwaysOn() const;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	void SetScale( const Vector3& scale );
	void AddTransformModifier( IEveChildTransformModifier * modifier ) override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;


	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectDecalOwner
	void AddDecal( EveSpaceObjectDecalPtr newDecal ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );
	virtual bool IsVisible( const EveUpdateContext& updateContext ) const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachmentOwner
	void AddAttachment( IEveSpaceObjectAttachment * attachment );
	void ClearAttachments();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveLightOwner
	virtual void GetLights( Tr2LightManager & lightManager ) const override;
	virtual void AddLight( Tr2Light * newLight ) override;
	virtual void ClearLights() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	bool IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const override;
	bool IsCastingShadow( const TriFrustum& cameraFrustum, Vector3 position, float radius, Tr2RenderReason renderReason ) const override;
	void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) override;
	Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;
	void PushRtGeometry( Tr2RaytracingManager & rtManager ) const override;
	void MarkRtDirty() override;
	bool IsShadowCastingDirty() const override;

	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// PerObjectData
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );
	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	IRoot* GetID( uint16_t ) override;
	void GetPickingBatches( ITriRenderBatchAccumulator * batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData ) override;

	// access
	void SetMesh( Tr2MeshBase * mesh );
	void SetOrigin( Origin origin );
	void SetReflectionMode( EntityComponents::ReflectionMode reflectionMode );
	void SetCastShadow( bool castShadow );
	void SetMinScreenSize( float minScreenSize );

	Tr2GrannyAnimation* GetAnimationController() const override;
	void SetAnimationController( Tr2GrannyAnimation * animation );

	void SetInstanceTransforms( std::vector<Matrix> instances );

	std::vector<std::string> GetMorphTargetNames() const;
	void SetMorphTargetWeight( const char* name, float weight );
	float GetMorphTargetWeight( const char* name );

	std::vector<bool> GetAllBakedMorphTargetStates() const;
	void SetBakedMorphTarget( const char* name, bool isBaked );
	bool GetBakedMorphTarget( const char* name );

	void BakeMorphs();
	void UnbakeMorphs();
	bool IsMeshBaked();

	bool UpdateMeshMorphs( Tr2RenderContext & renderContext ) override;

	bool IsMorphsBaked() const;
	BluePy GetSofSourceLocator( uint32_t areaId ) const;

protected:
	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

	void InitializeAnimation();
	bool ShouldReflect() const;

	bool DisplayDecals() const;

	bool PrepareMorphBuffers( Tr2RenderContext & renderContext );

	std::pair<const Float4x3*, size_t> GetBoneTransforms() const;
	const std::pair<const int32_t*, size_t> GetMeshBindingIndices() const;
	std::pair<const Tr2MorphTargetAnimationData*, size_t> GetMorphTargets( MorphTargetAnimationFilter filter );
	void UpdateMorphAnimationBuffer();

	// general data
	BlueSharedString m_name;

	// the mesh
	Tr2MeshBasePtr m_mesh;
	Tr2InstancedMeshPtr m_instancedMesh; // Cached downcast of m_mesh
	IEveSpaceObject2::ParentData m_parentData;

	PIEveChildTransformModifierVector m_transformModifiers;
	Tr2GrannyAnimationPtr m_animationUpdater;
	std::unique_ptr<Tr2AnimationMeshBinding> m_meshBinding;

	Tr2Lod m_lowestLodVisible;

	float m_minScreenSize;
	float m_currentScreenSize;
	float m_currentInstanceScreenSize;

	float m_sortValueOffset;
	float m_sortValueScale;

	// per-object data
	Tr2RingBufferOffsets m_boneOffsets;
	Tr2RingBufferOffsets m_morphTargetOffsets;
	Tr2PersistentPerObjectData<EveChildMesh> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveChildMesh> m_perObjectDataPs;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;

	CcpMath::AxisAlignedBox m_worldBoundingBox; // AABB in world space
	CcpMath::Sphere m_worldBoundingSphere; // bounding sphere in world space

	bool m_display;
	bool m_isVisible;
	bool m_instancesVisible;
	bool m_castShadow;
	bool m_updateAnimation;
	bool m_bakeMorphs;
	bool m_dirtyRtMesh;
	// Has UpdateSyncronous/UpdateAsyncronous been called: until it has, the object cannot be rendered
	bool m_hasUpdated = false;

	float m_activationStrength;

	Origin m_origin;

	EntityComponents::ReflectionMode m_reflectionMode;

	PEveSpaceObjectDecalVector m_decals;
	PIEveSpaceObjectAttachmentVector m_attachments;
	PTr2LightVector m_lights;

	bool m_allowAudioGeometry;
	bool m_audioGeometryRegistered;
	uint64_t m_audioGeometrySetId;
	uint64_t m_audioInstanceId;
	ITr2AudGeometryPtr m_audioGeometry;

	void RegisterAudioGeometry();
	void UnregisterAudioGeometry();

	void UpdateRtMesh();
	void UpdateRtSkeleton();
	mutable Tr2ConstantBufferAL m_rtPerObjectData;
	std::vector<Matrix> m_instanceTransforms;
	mutable std::vector<Float4x3> m_instanceWorldTransforms;
	unsigned int m_instanceCount;

	std::vector<Tr2MorphTargetAnimationData> m_morphAnimationBuffer;
	std::unordered_map<std::string, Tr2MorphTargetAnimationData> m_morphAnimationData;
	Tr2SuballocatedBuffer::Allocation m_bakedMorphAllocation;
	Tr2EffectPtr m_mergeMorphsEffect;
	Tr2ConstantBufferAL m_mergeMorphsConstantBuffer;
	bool m_isMorphsBaked;
	struct
	{
		uint32_t m_runtimeEvaluatedOffset;
		uint32_t m_runtimeEvaluatedCount;
		uint32_t m_bakedOffset;
		uint32_t m_bakedCount;
		uint32_t m_allCount;
	} m_morphAnimationOffsets;
};

TYPEDEF_BLUECLASS( EveChildMesh );

#endif
