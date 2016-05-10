////////////////////////////////////////////////////////////
//
//    Created:   February 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveCloud_H
#define EveCloud_H

#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Tr2Renderer.h"
#include "Tr2ShLightingManager.h"

BLUE_DECLARE( EveCloudEditableVolume );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );

// --------------------------------------------------------------------------------
// Description:
//   A space object used for rendering volumetric clouds. Renders a camera-aligned
//   plane at the back of the object's bounding sphere using provided effect.
// --------------------------------------------------------------------------------
BLUE_CLASS( EveCloud ) :
	public ITr2Renderable,
	public ITr2Pickable,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify,
	public IEveSpaceObjectChild
{
public:
	EXPOSE_TO_BLUE();

	EveCloud(IRoot* lockobj = NULL);
	~EveCloud();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	virtual void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	virtual void GetLocalToWorldTransform( Matrix &transform ) const;
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform, Tr2Lod parentLod );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	virtual void PlayCurveSet( const std::string& name );
	virtual void StopCurveSet( const std::string& name );
	virtual float GetCurveSetDuration( const std::string& name ) const; 
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) {}
	virtual void ChangeLOD( Tr2Lod lod ) {};

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
    virtual IRoot* GetID( uint16_t areaId );
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

private:
	bool OnPrepareResources();

	// data for positioning
	Matrix m_localTransform;
	Matrix m_worldTransform;

	// bounding sphere
	Vector4 m_boundingSphere;

	ITr2ShaderMaterialPtr m_effect;

	Vector3 m_scaling;
	Vector3 m_translation;
	Quaternion m_rotation;

	uint32_t m_preTesselationLevel;

	uint32_t m_declaration;
	Tr2VertexBufferAL m_vertexBuffer;
	Tr2IndexBufferAL m_indexBuffer;

	std::string m_name;
	bool m_display;

	EveCloudEditableVolumePtr m_volume;

	Vector3 m_min;
	Vector3 m_max;

	float m_sortingModifier;
};

TYPEDEF_BLUECLASS( EveCloud );
BLUE_DECLARE_VECTOR( EveCloud );

#endif