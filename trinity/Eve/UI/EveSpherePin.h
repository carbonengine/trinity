// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EVESPHEREPIN_H
#define EVESPHEREPIN_H


#include "ITr2Renderable.h"
#include "Eve/IEveTransform.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( EveSpherePin );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriGeometryRes );

class EveSpherePinIndexTree;

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for spherepins
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveSpherePinPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;
	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override;

	// per object data (shared to VS and PS)
	Matrix m_worldMatrix;
	Vector4 m_pinPosition;
	Vector4 m_pinRotation;
	Vector4 m_pinColor;
	Vector4 m_pinThreshold;
	Vector4 m_pinRadiusPrecalc;
	Vector4 m_pinUV;
};


// --------------------------------------------------------------------------------
// Description:
//   An EveSpherePin is a very basic object that is just a circle on a sphere. It
//   is used in a variety of places, mostly to do with "decals on planets"
// SeeAlso:
//   EvePlanet
// --------------------------------------------------------------------------------
class EveSpherePin : public IInitialize,
					 public ITr2Renderable,
					 public IEveTransform,
					 public IEveSpaceObject2,
					 public Tr2DeviceResource,
					 public IBlueAsyncResNotifyTarget,
					 public ITr2Pickable,
					 public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveSpherePin( IRoot* lockobj = NULL );
	~EveSpherePin();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveTransform
	virtual void Update( const EveUpdateContext& updateContext );
	virtual void UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform );
	Tr2Lod GetLODLevel() const
	{
		return TR2_LOD_HIGH;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	// No sensible implementation?
	virtual void UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
	{
	}
	virtual void GetModelCenterWorldPosition( Vector3& position ) const
	{
	}
	virtual bool GetLocalBoundingBox( Vector3& min, Vector3& max )
	{
		return false;
	}
	virtual void GetLocalToWorldTransform( Matrix& transform ) const
	{
		transform = IdentityMatrix();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t )
	{
		return this->GetRawRoot();
	}
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	void GetBatchWithEffect( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Effect* effect );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p );
	virtual void RebuildCachedData( BlueAsyncRes* p );

	// geom res
	void InitializeGeometryResource();

private:
	// name
	std::string m_name;

	// transforms of this set
	Vector3 m_scaling;
	Quaternion m_rotation;
	Vector3 m_translation;
	// resulting transform
	Matrix m_worldTransform;

	// data on the pin
	Vector3 m_centerNormal;
	float m_pinMaxRadius;
	float m_pinRadius;
	float m_pinRotation;
	Color m_pinColor;
	float m_pinAlphaThreshold;

	// uv atlasing
	Vector4 m_uvAtlasScaleOffset;

	// pin draw shader is set by python by name
	std::string m_pinEffectResPath;
	Tr2EffectPtr m_pinEffect;
	// picking shader is unchangable
	bool m_enablePicking;
	Tr2EffectPtr m_pickEffect;

	// shared resource: all pins share the same (should be also the same planets use!)
	std::string m_geomResPath;
	TriGeometryResPtr m_geometryResource;

	// culling
	Vector4 m_boundingSphere;
	bool m_display;
	float m_sortValueMultiplier;
	void BuildBoundingSphere();

	// animated via curves
	PTriCurveSetVector m_curveSets;

	int m_primitiveCount;
	Tr2BufferAL m_indexBuffer;
	void CreateIndexBuffer();

	int m_rebuildIndices;
	EveSpherePinIndexTree* m_tree;
};

TYPEDEF_BLUECLASS( EveSpherePin );
BLUE_DECLARE_VECTOR( EveSpherePin );

#endif