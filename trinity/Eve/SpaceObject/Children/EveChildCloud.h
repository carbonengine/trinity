// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveChildCloud_H
#define EveChildCloud_H

#include "ITr2Renderable.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Tr2ShLightingManager.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveCloudEditableVolume );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );

// --------------------------------------------------------------------------------
// Description:
//   A space object used for rendering volumetric clouds. Renders a camera-aligned
//   plane at the back of the object's bounding sphere using provided effect.
// --------------------------------------------------------------------------------
BLUE_CLASS( EveChildCloud ) :
	public ITr2Renderable,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify,
	public IEveSpaceObjectChild,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveChildCloud( IRoot* lockobj = NULL );
	~EveChildCloud();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	virtual const char* GetName() const;
	virtual void SetName( const char* name );
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	virtual void GetLocalToWorldTransform( Matrix & transform ) const;
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	virtual bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
	{
	}
	virtual void ChangeLOD( Tr2Lod lod ) {};

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

private:
	bool OnPrepareResources();

	// data for positioning
	Matrix m_localTransform;
	Matrix m_worldTransform;

	// bounding sphere
	Vector4 m_boundingSphere;

	Tr2MaterialPtr m_effect;

	Vector3 m_scaling;
	Vector3 m_translation;
	Quaternion m_rotation;

	uint32_t m_preTesselationLevel;

	uint32_t m_declaration;
	Tr2BufferAL m_vertexBuffer;
	TrackableStdVector<Tr2BufferAL> m_indexBuffers;

	std::string m_name;
	bool m_display;
	bool m_isVisible;
	// Has UpdateSyncronous/UpdateAsyncronous been called: until it has, the object cannot be rendered
	bool m_hasUpdated = false;

	EveCloudEditableVolumePtr m_volume;

	Vector3 m_min;
	Vector3 m_max;

	float m_sortingModifier;
	float m_minScreenSize;

	float m_cellScreenSize;
	size_t m_currentIB;

	float m_lastLodFactor;
};

TYPEDEF_BLUECLASS( EveChildCloud );
BLUE_DECLARE_VECTOR( EveChildCloud );

#endif