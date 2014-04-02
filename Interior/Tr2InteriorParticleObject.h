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

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	void SetVisibility( bool bVisible );
	bool IsVisible( void ) const;

	void SetVisibleLightCount( int visibleLights );
	void SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet );

	Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix 
	);
	Tr2PerObjectData* GetPerObjectDataForPrePass(
		ITriRenderBatchAccumulator* accumulator,
		const Matrix& objectToWorldMatrix
	);

	void SetMirrorDepth( int depth ) {}
	void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver );

	// Bounding sphere
	bool GetBoundingSphere( Vector4& sphere ) const;
	bool GetLocalBoundingBox( Vector3& min, Vector3& max ) const;
	bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;
	bool IsBoundingBoxReady( void ) const;
	bool GetShProbePosition( Vector3& position ) const;

	// Spherical harmonics update
	virtual void PrePhysicsUpdate( Be::Time time );
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene );
	virtual void SetSHSampleIndex( unsigned int index ) { m_shSampleIndex = index; }
	virtual unsigned int GetSHSampleIndex() const  { return m_shSampleIndex; }
	virtual Matrix& GetRedLightProbeMatrix( void );
	virtual Matrix& GetGreenLightProbeMatrix( void );
	virtual Matrix& GetBlueLightProbeMatrix( void );

	// Debug visualization
	bool DoVisualizeLightProbes( void ) const;

	// Scene add/remove
	bool AddToScene( Tr2ApexScene *apexScene );
	void RemoveFromScene( void );

	// Umbra interaction
	bool IsUmbraReady( void ) const;
	bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell );
	virtual void CellRemoved( Tr2InteriorCell* cell );
	bool IsDirty( void ) const;
	void ClearDirty( void );

	// Set the dirty flag
	void SetDirtyFlag( bool isDirty );
	bool IsBackgroundProxy( void ) const;
	void AddToCellAsBackgroundProxy( Umbra::Cell* cell );
	void AddToRootCell( Umbra::Cell* cell );
	bool IsShadowCaster( void ) const;

	void UpdateUmbraObject( Umbra::Cell* cell, Umbra::Object*& object ) const;

	// LOD
	void SetLOD( const TriFrustum* frustum );

	bool CastsShadows() const { return false; }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, 
							 TriBatchType batchType, 
							 const Tr2PerObjectData* perObjectData );

    bool HasTransparentBatches();
    float GetSortValue(); 

	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	const ITr2RenderableVector* GetAttachedRenderables() { return NULL; };

	void RenderDebugInfo( TriLineSet* lines ) const;
	void BindLowLevelShaders();

	//////////////////////////////////////////////////////////////////////////
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front_, const Vector3& top_, const Vector3& pos_ );

private:
	void ClearUmbra();
	void RebuildVolume();

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
	// Flag to visualize SH light probes
	bool m_visualizeLightProbes;
	// Umbra bounding objects
	std::vector<Umbra::Object*> m_umbraObjects;
	// Umbra bounding object model
	Umbra::Model* m_umbraModel;
	// Transform from umbra object space to local space
	Matrix m_invBoundingBoxScale;

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
	// index of SH task in Enlighten task manager
	unsigned int m_shSampleIndex;
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