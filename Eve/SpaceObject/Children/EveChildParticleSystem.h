////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildParticleSystem_H
#define EveChildParticleSystem_H

#include "IEveSpaceObjectChild.h"
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
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	EveChildParticleSystem( IRoot* lockobj = NULL );
	~EveChildParticleSystem();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );

	void PlayCurveSet( const std::string& name ) {};
	void StopCurveSet( const std::string& name ) {};
	float GetCurveSetDuration( const std::string& name ) const { return 0; } 
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches();
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

private:
	BlueSharedString m_name;
	bool m_display;

	Vector3 m_scaling;
	Vector3 m_translation;
	Quaternion m_rotation;
	Matrix m_worldTransform;

	Vector4 m_boundingSphere;

	Tr2InstancedMeshPtr m_mesh;
	PTr2ParticleSystemVector m_particleSystems;
	PITr2GenericEmitterVector m_particleEmitters;
};

TYPEDEF_BLUECLASS( EveChildParticleSystem );

#endif