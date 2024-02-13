////////////////////////////////////////////////////////////
//
//    Created:   August 2015
//    Copyright: CCP 2015
//

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
#include "Resources/Tr2LodResource.h"
#include "TransformModifiers/IEveChildTransformModifier.h"
#include "Tr2DebugRenderer.h"


BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( EveSpaceObject2 );

BLUE_CLASS( EveChildMesh ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize,
	public ITr2DebugRenderable,
	public ITr2GrannyAnimationOwner,
	public EveEntity,
	public INotify,
	public IEveSpaceObjectDecalOwner,
	public IEveSpaceObjectAttachmentOwner,
	public ITr2LightOwner,
	public IEveShadowCaster
{
public:
	EXPOSE_TO_BLUE();

	EveChildMesh( IRoot* lockobj = NULL );
	~EveChildMesh();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod ) override;
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	bool IsAlwaysOn() const;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	void SetScale( const Vector3& scale );
	void AddTransformModifier( IEveChildTransformModifier* modifier ) override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;

	
	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectDecalOwner
	void AddDecal( EveSpaceObjectDecalPtr newDecal ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	virtual bool IsVisible( const TriFrustum& frustum ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachmentOwner
	void AddAttachment( IEveSpaceObjectAttachment* attachment );
	void ClearAttachments();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveLightOwner
	virtual void GetLights( Tr2LightManager& lightManager ) const;
	virtual void AddLight( Tr2Light* newLight );
	virtual void ClearLights();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	bool IsCastingShadow( const TriFrustum& cameraFrustum, const TriFrustumOrtho& shadowFrustum, const uint32_t shadowMapSize, const Vector3 sunDir, float& sizeInShadow ) const override;
	void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) override;
	Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;

	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// PerObjectData
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );
	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;

	// access
	void SetMesh( Tr2MeshBase* mesh );
	void SetOrigin( Origin origin );
	void SetReflectionMode( EntityComponents::ReflectionMode reflectionMode );
	void SetCastShadow( bool castShadow );

	Tr2GrannyAnimation* GetAnimationController() const override;
	void SetAnimationController( Tr2GrannyAnimation* animation );

protected:
	void InitializeAnimation();
	bool ShouldReflect() const;


	// general data
	BlueSharedString m_name;

	// the mesh
	Tr2MeshBasePtr m_mesh;
	IEveSpaceObject2::ParentData m_parentData;

	PIEveChildTransformModifierVector m_transformModifiers;
	Tr2GrannyAnimationPtr m_animationUpdater;

	Tr2Lod m_lowestLodVisible;

	float m_minScreenSize;
	float m_currentScreenSize;

	float m_sortValueOffset;
	float m_sortValueScale;

	// per-object data
	Tr2BoneTransformOffsets m_boneOffsets;
	Tr2PersistentPerObjectData<EveChildMesh> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveChildMesh> m_perObjectDataPs;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;
	
	bool m_display;
	bool m_isVisible;
	bool m_castShadow;

	float m_activationStrength;

	Origin m_origin;

	EntityComponents::ReflectionMode m_reflectionMode;

	PEveSpaceObjectDecalVector m_decals;
	PIEveSpaceObjectAttachmentVector m_attachments;
	PTr2LightVector m_lights;
};

TYPEDEF_BLUECLASS( EveChildMesh );

#endif