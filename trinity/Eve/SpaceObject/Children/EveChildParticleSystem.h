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
#include "TransformModifiers/IEveChildTransformModifier.h"
#include "Tr2DebugRenderer.h"


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
	public IInitialize,
	public ITr2DebugRenderable,
	public INotify,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildParticleSystem( IRoot* lockobj = NULL );
	~EveChildParticleSystem();
	
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
	void ChangeLOD( Tr2Lod lod );
	void PlayCurveSet( const std::string& name, const std::string& rangeName ) {};
	void StopCurveSet( const std::string& name ) {};
	float GetCurveSetDuration( const std::string& name ) const { return 0; } 
	void GetLights( Tr2LightManager& lightManager ) const {};
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void AddTransformModifier( IEveChildTransformModifier* modifier ) override;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches();
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	bool IsVisible( const TriFrustum& frustum ) const;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();
		
	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value );
	
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	PITr2GenericEmitterVector m_particleEmitters;
private:
	BlueSharedString m_name;

	Vector4 m_boundingSphere;
	Vector4 m_lodSphere;

	Tr2InstancedMeshPtr m_mesh;
	PTr2ParticleSystemVector m_particleSystems;
	PIEveChildTransformModifierVector m_transformModifiers;

	// Lodding
	bool m_useDynamicLod;
	float m_lodFactorMedium;
	float m_lodFactorLow;
	uint32_t m_lodClampLow;

	float m_lodSphereRadius;
	float m_minScreenSize;
	float m_currentScreenSize;

	bool m_display;
	bool m_isVisible;

	EntityComponents::ReflectionMode m_reflectionMode;
};

TYPEDEF_BLUECLASS( EveChildParticleSystem );

#endif