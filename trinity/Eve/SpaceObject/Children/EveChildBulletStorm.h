// Copyright © 2015 CCP ehf.

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
	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override;

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
	bool OnModified( Be::Var * val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
	{
	}
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches();
	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
	// rebuild the internal data
	void Rebuild();
	void StartEffect();
	void StopEffect();
	bool CanChangeState();


private:
	// general data
	std::string m_name;
	bool m_display;
	// Has UpdateSyncronous/UpdateAsyncronous been called: until it has, the object cannot be rendered
	bool m_hasUpdated = false;

	// bullets data
	unsigned int m_objectCount;
	uint32_t m_multiplier;
	std::string m_sourceLocatorSet;
	float m_range;
	float m_speed;
	float m_clipSphere;
	float m_clipShereMul;
	bool m_changingClipSphere;

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
	Tr2BufferAL m_vertexBuffer;
	Tr2BufferAL m_perInstanceBuffer;

	// where is our parent
	Matrix m_worldTransform;
};

TYPEDEF_BLUECLASS( EveChildBulletStorm );

#endif