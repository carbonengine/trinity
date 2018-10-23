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
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Resources/Tr2LodResource.h"
#include "TransformModifiers/IEveChildTransformModifier.h"

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( EveSpaceObject2 );

BLUE_CLASS( EveChildMesh ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize
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
	void ChangeLOD( Tr2Lod lod ) {};
	void GetLights( Tr2LightManager& lightManager ) const {};
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	bool IsAlwaysOn() const;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// PerObjectData
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );
	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;

	// access
	void SetMesh( Tr2MeshBase* mesh );
	void SetOrigin( Origin origin );

protected:
	// general data
	BlueSharedString m_name;

	// the mesh
	Tr2MeshBasePtr m_mesh;

	PIEveChildTransformModifierVector m_transformModifiers;

	Tr2Lod m_lowestLodVisible;

	float m_minScreenSize;
	float m_currentScreenSize;

	float m_sortValueOffset;
	float m_sortValueScale;

	// per-object data
	Tr2PersistentPerObjectData<EveChildMesh> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveChildMesh> m_perObjectDataPs;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;
	
	bool m_display;
	bool m_isVisible;
	bool m_useSpaceObjectData;

	Origin m_origin;
};

TYPEDEF_BLUECLASS( EveChildMesh );

#endif