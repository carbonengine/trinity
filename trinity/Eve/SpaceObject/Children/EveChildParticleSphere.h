////////////////////////////////////////////////////////////
//
//    Created:   February 2018
//    Copyright: CCP 2018
//

#pragma once

#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "ITr2Renderable.h"
#include "Particle/Tr2ParticleElementDeclaration.h"

BLUE_DECLARE( Tr2InstancedMesh );
BLUE_DECLARE_INTERFACE( ITr2AttributeGenerator );
BLUE_DECLARE_IVECTOR( ITr2AttributeGenerator );
BLUE_DECLARE( Tr2ParticleSystem );
class EveUpdateContext;

BLUE_CLASS( EveChildParticleSphere ) :
	public IEveSpaceObjectChild,
	public ITr2Renderable
{
public:
	EveChildParticleSphere( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();


	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void ChangeLOD( Tr2Lod lod ) {};
	void GetLights( Tr2LightManager& lightManager ) const {};

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	bool HasTransparentBatches();
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	bool CheckBinding();

	void Refresh();
private:
	void Update( const EveUpdateContext& updateContext );
	void ApplyConstraint( const Vector3& previousReferencePosition, const Vector3& velocityDirection );
	void AddParticles( const Vector3& previousReferencePosition, const Vector3& velocityDirection );
	void FillParticleData( float** particle, const Vector3& previousReferencePosition, const Vector3& velocityDirection );

	Tr2InstancedMeshPtr m_mesh;
	Tr2ParticleSystemPtr m_particleSystem;
	PITr2AttributeGeneratorVector m_generators;

	// Particle element data for position
	Tr2ParticleElementData m_positionElement;
	// Particle element data for velocity
	Tr2ParticleElementData m_velocityElement;
	// Particle element data for particle lifetime
	Tr2ParticleElementData m_lifetimeElement;

	Matrix m_worldTransform;
	Vector4 m_boundingSphere;

	std::string m_name;

	enum
	{
		BIND_PENDING,
		BIND_VALID,
		BIND_INVALID,
	} m_bindStatus;

	Vector3 m_previousOrigin;

	// Radius of the effect sphere - particles outise are deleted
	float m_radius;

	// Amount that particles move is scaled by this value
	float m_movementScale;
	// Reference movement is clamped at this speed
	float m_maxSpeed;

	// ego velocity
	Vector3 m_egoVelocity;
	// ego speed
	float m_egoSpeed;

	float m_positionShift;
	float m_positionShiftMax;
	float m_positionShiftMin;
	float m_positionShiftNormalized;

	float m_positionShiftIncreaseSpeed;
	float m_positionShiftDecreaseSpeed;

	bool m_useSpaceObjectData;
	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildParticleSphere );