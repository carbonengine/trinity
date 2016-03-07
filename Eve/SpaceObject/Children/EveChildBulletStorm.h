////////////////////////////////////////////////////////////
//
//    Created:   December 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveChildBulletStorm_H
#define EveChildBulletStorm_H

#include "IEveSpaceObjectChild.h"
#include "ITr2Renderable.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2PersistentPerObjectData.h"

// forwards
BLUE_DECLARE_IVECTOR( IEveSpaceObject2 );

// --------------------------------------------------------------------------------
// Description:
//   Very specialised per-pbject data
// --------------------------------------------------------------------------------
class EveChildBulletStormPerObjectData : public Tr2PerObjectData
{
public:
	void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	// vs
	Matrix m_worldTransform;
	Vector4 m_effectInfo;
	Vector4 m_targetPositionsWS[10];
};

// --------------------------------------------------------------------------------
// Description:
//   A spaceobject child for rendering swarm like bullets
// --------------------------------------------------------------------------------
BLUE_CLASS( EveChildBulletStorm ) :
	public IEveSpaceObjectChild,
	public ITr2Renderable,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveChildBulletStorm( IRoot* lockobj = NULL );
	~EveChildBulletStorm();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform, Tr2Lod parentLod );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};
	void PlayCurveSet( const std::string& name );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches();
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:
	// rebuild the internal data
	void Rebuild();


private:
	// general data
	std::string m_name;
	bool m_display;

	// bullets data
	unsigned int m_objectCount;
	unsigned int m_multiplier;
	std::string m_sourceLocatorSet;
	float m_range;

	// source
	EveSpaceObject2Ptr m_sourceObject;
	float m_sourceRadius;

	// targets
	PIEveSpaceObject2Vector m_targetObjects;
	std::vector<Vector4> m_targetBlobs;

	// this shader renders them all
	Tr2EffectPtr m_effect;

	// has it's own vertex handle and buffer
	uint32_t m_vertexDeclHandle;
	Tr2VertexBufferAL m_vertexBuffer;
	Tr2VertexBufferAL m_perInstanceBuffer;

	// where is our parent
	Matrix m_worldTransform;
};

TYPEDEF_BLUECLASS( EveChildBulletStorm );

#endif