// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveEllipseSet_H
#define EveEllipseSet_H

#include "Eve/SpaceObject/Children/EveChildTransform.h"
#include "ITr2Renderable.h"
#include "Tr2DeviceResource.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2EffectStateManager.h"
#include "Tr2BufferAL.h"
#include "EveEllipseDefinition.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

BLUE_DECLARE( EveEllipseSet );

BLUE_CLASS( EveEllipseSet ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IListNotify,
	public INotify,
	public Tr2DeviceResource,
	public ITr2Pickable
{
public:
	EXPOSE_TO_BLUE();

	EveEllipseSet( IRoot* lockobj = nullptr );
	~EveEllipseSet() override;

	void py__init__();

	void ClearEllipses();
	bool AddEllipse( const Vector3& center, float semiMajor, float semiMinor, const Vector3& planeNormal, float rotationDegrees );

	//////////////////////////////////////////////////////////////////////////////////////
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

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	IRoot* GetID( uint16_t ) override;
	void GetPickingBatches( ITriRenderBatchAccumulator * batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	bool HasTransparentBatches() override;
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Tr2DeviceResource
	void ReleaseResources( TriStorage s ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val ) override;

	BlueSharedString m_name;
	bool m_display = true;
	bool m_enablePicking = true;
	float m_depthOffset = 0.f;
	unsigned int m_ribbonSegmentCount = 128u;
	Tr2Lod m_lowestLodVisible = TR2_LOD_LOW;

	Tr2EffectPtr m_effect;

private:
	void CommitRibbonBatch( ITriRenderBatchAccumulator * accumulator, const Tr2PerObjectData* perObjectData, Tr2EffectPtr& effect );

	bool OnPrepareResources() override;

	void MarkGeometryDirty();
	void RefreshGeometryIfNeeded( const EveUpdateContext& updateContext );
	void RebuildGeometry( const EveUpdateContext& updateContext );

	PEveEllipseDefinitionVector m_ellipses;

	Vector4 m_boundingSphere{};
	bool m_isVisible = false;

	bool m_geometryDirty = true;

	Tr2BufferAL m_vertexBuffer;
	Tr2BufferAL m_indexBuffer;
	unsigned int m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	unsigned int m_indexCount = 0;
};

TYPEDEF_BLUECLASS( EveEllipseSet );

#endif
