////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildParticleSystem_H
#define EveChildParticleSystem_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2InstancedMesh );
BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( EveSpaceObject2 );

BLUE_DECLARE( Tr2ParticleSystem );
BLUE_DECLARE_VECTOR( Tr2ParticleSystem );
BLUE_DECLARE_INTERFACE( ITr2GenericEmitter );
BLUE_DECLARE_IVECTOR( ITr2GenericEmitter );

BLUE_CLASS( EveChildParticleSystem ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveChildParticleSystem( IRoot* lockobj = NULL );
	~EveChildParticleSystem();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent );
	void UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent );
	void GetLocalToWorldTransform( Matrix& transform ) const;

	void PlayCurveSet( const std::string& name ) {};
	void StopCurveSet( const std::string& name ) {};
	float GetCurveSetDuration( const std::string& name ) const { return 0; } 
	
	virtual void Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation ) { EveChildTransform::Transform( scale, rotation, translation ); }
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches();
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

private:
	BlueSharedString m_name;
	bool m_display;

	Vector4 m_boundingSphere;

	Tr2InstancedMeshPtr m_mesh;
	PTr2ParticleSystemVector m_particleSystems;
	PITr2GenericEmitterVector m_particleEmitters;
};

TYPEDEF_BLUECLASS( EveChildParticleSystem );

#endif