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
	public IEveSpaceObject2,
	public ITr2Renderable,
	public ITr2Pickable,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify,
	public IEveSpaceObjectChild,
	public ITr2SecondaryLightSource
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
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void RenderDebugInfo( Tr2RenderContext& renderContext );
	void GetModelCenterWorldPosition( Vector3 &position, Be::Time t );
	void GetCurrentModelCenterWorldPosition( Vector3 &position );
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	void GetLocalToWorldTransform( Matrix &transform ) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	virtual void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform, Tr2Lod parentLod );
	virtual void PlayCurveSet( const std::string& name );
	virtual void StopCurveSet( const std::string& name );
	virtual float GetCurveSetDuration( const std::string& name ) const; 
	virtual void Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation ) {}
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

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2SecondaryLightSource
	virtual void RegisterSecondaryLightSource( Tr2ShLightingManager& );
	virtual void UnregisterSecondaryLightSource( Tr2ShLightingManager& );

private:
	bool OnPrepareResources();

	// data for positioning
	Matrix m_localTransform;
	Matrix m_worldTransform;

	// bounding sphere
	Vector4 m_boundingSphere;

	ITr2ShaderMaterialPtr m_effect;

	// The ability to attach the LineSet to a Ball
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	Vector3 m_scaling;

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

	float m_secondaryLightingSphereRadiusLocal;
	float m_secondaryLightingSphereRadiusWorld;
	Color m_secondaryLightingEmissiveColor;
};

TYPEDEF_BLUECLASS( EveCloud );
BLUE_DECLARE_VECTOR( EveCloud );

#endif