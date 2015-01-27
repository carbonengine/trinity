////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2InteriorParticleObject_H
#define Tr2InteriorParticleObject_H

#include "include/ITr2Interior.h"
#include "Utilities/BoundingBox.h"

BLUE_DECLARE_INTERFACE( ITr2GenericEmitter );
BLUE_DECLARE_IVECTOR( ITr2GenericEmitter );
BLUE_DECLARE( Tr2ParticleSystem );
BLUE_DECLARE_VECTOR( Tr2ParticleSystem );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE_VECTOR( Tr2Mesh );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );

// -------------------------------------------------------------
// Description:
//   An interior dynamic object that contains particle systems.
// SeeAlso:
//   Tr2InteriorSpriteParticleSystem, ITr2InteriorDynamic
// -------------------------------------------------------------
class Tr2InteriorParticleObject:
	public ITr2InteriorDynamic, 
	public ITr2Renderable,
	public IBluePlacementObserver,
	public IInitialize
{
public:
	Tr2InteriorParticleObject( IRoot* lockobj = 0 );
	~Tr2InteriorParticleObject();

	EXPOSE_TO_BLUE();

    using ITr2Renderable::Lock;
    using ITr2Renderable::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	void SetVisibility( bool bVisible );
	bool IsVisible( void ) const;

	Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix 
	);

	void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver );

	bool GetLocalBoundingBox( Vector3& min, Vector3& max ) const;
	bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;
	bool IsBoundingBoxReady( void ) const;
	bool GetShProbePosition( Vector3& position ) const;

	// Spherical harmonics update
	virtual void PrePhysicsUpdate( Be::Time time );
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene );
	virtual Matrix& GetRedLightProbeMatrix( void );
	virtual Matrix& GetGreenLightProbeMatrix( void );
	virtual Matrix& GetBlueLightProbeMatrix( void );

	// Scene add/remove
	bool AddToScene( Tr2ApexScene *apexScene );
	void RemoveFromScene( void );

	bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell );
	bool IsDirty( void ) const;
	void SetDirtyFlag( bool isDirty );
	bool IsShadowCaster( void ) const;

	// LOD
	void SetLOD( const TriFrustum* frustum );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, 
							 TriBatchType batchType, 
							 const Tr2PerObjectData* perObjectData );

    bool HasTransparentBatches();
    float GetSortValue(); 

	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	void RenderDebugInfo( TriLineSet* lines ) const;
	void BindLowLevelShaders();

	//////////////////////////////////////////////////////////////////////////
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front_, const Vector3& top_, const Vector3& pos_ );

private:
	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;
	AxisAlignedBoundingBox GetBoundingBoxInWorldSpace() const;

	// Python-exposed object name
	std::string m_name;
	// Particle systems rendered by this object
	PTr2ParticleSystemVector m_particleSystems;
	// Particle emitters owned by this object
	PITr2GenericEmitterVector m_emitters;
	// Instanced meshes
	PTr2MeshVector m_meshes;
	// Local to world space transform
	Matrix m_transform;

	// SH lighting solver for transparent rendering
	ITr2InteriorSHLightingSolver *m_shSolver;

	// Max particle radius (to calculate correct bounding box)
	float m_maxParticleRadius;

	// Explicit bounding box for SH lighting
	Vector3 m_shBoundsMin;
	Vector3 m_shBoundsMax;

	// Depth offset for transparency sorting
	float m_depthOffset;

	// Visible flag
	bool m_isVisible;
	// Red SH probe coefficients
	Matrix m_redProbeMatrix;
	// Green SH probe coefficients
	Matrix m_greenProbeMatrix; 
	// Blue SH probe coefficients
	Matrix m_blueProbeMatrix;

	// Render debug info
	bool m_renderDebugInfo;

	PTriCurveSetVector m_curveSets;

	// Cached mirror-to-world matrix from last GetPerObjectData call
	Matrix m_mirrorToWorldMatrix;
};

TYPEDEF_BLUECLASS( Tr2InteriorParticleObject );
BLUE_DECLARE_VECTOR( Tr2InteriorParticleObject );

#endif // Tr2InteriorParticleObject_H