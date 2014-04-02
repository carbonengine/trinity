#pragma once
#ifndef Tr2InteriorFlare_H
#define Tr2InteriorFlare_H

#include "include/ITr2Interior.h"
#include "Tr2DeviceResource.h"
#include "Utilities/BoundingBox.h"

BLUE_DECLARE( Tr2ShaderMaterial );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( Tr2InteriorFlareData );
BLUE_DECLARE_VECTOR( Tr2InteriorFlareData );
BLUE_DECLARE( TriViewport );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorFlare represents a flare object for interiors. Tr2InteriorFlare is an
//   interior dynamic object that renders flare billboards. It runs D3D occlusion
//   queries to determine flare visibility.
// See Also:
//   Tr2InteriorScene
// --------------------------------------------------------------------------------------
class Tr2InteriorFlare:
	public ITr2InteriorDynamic, 
	public ITr2Renderable,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify,
	public IBluePlacementObserver
{
public:
	Tr2InteriorFlare( IRoot* lockobj = 0 );
	~Tr2InteriorFlare();

	EXPOSE_TO_BLUE();

    using ITr2Renderable::Lock;
    using ITr2Renderable::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	void SetVisibility( bool bVisible );
	bool IsVisible( void ) const;

	void SetVisibleLightCount( int visibleLights ) {}
	void SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet ) {}

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
	void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver ) { }

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

	virtual bool CastsShadows() const { return false; }

	// Debug visualization
	bool DoVisualizeLightProbes( void ) const { return false; }

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

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, 
							 TriBatchType batchType, 
							 const Tr2PerObjectData* perObjectData );

    bool HasTransparentBatches();
    float GetSortValue(); 

	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	const ITr2RenderableVector* GetAttachedRenderables() { return NULL; };

	//////////////////////////////////////////////////////////////////////////
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front_, const Vector3& top_, const Vector3& pos_ );

	Vector3 GetPosition() const;
	void SetPosition( const Vector3& position );

	void OverrideViewport( TriViewport* viewport );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void ReleaseResources( TriStorage s );
protected:
	bool OnPrepareResources();
private:
	// Types of flare rendering
	enum FlareType
	{
		// Occlusion billboard
		OCCLUSION_FLARE,
		// Flare billboards during material pass
		TRANSPARENT_FLARE,
		// Flare billboards during flare pass
		NON_POSTPROCCESSED_FLARE,
	};

	void CreateOcclusionObjects();
	void UpdateFlareVisibility( ITriRenderBatchAccumulator* batches );
	void GetFlareBatches( ITriRenderBatchAccumulator* batches, FlareType type );
	void RenderFlare( FlareType type, Tr2RenderContext& renderContext );
	void ClearUmbra();
	void RebuildVolume();
	void UpdateUmbraObjectTransform();

	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;
	AxisAlignedBoundingBox GetBoundingBoxInWorldSpace() const;

	// Python-exposed object name
	std::string m_name;
	// Color of the flare
	Color m_color;
	// Visibility flag
	bool m_isVisible;

	// Flare position in world space
	Matrix m_transform;
	// Umbra bounding object
	Umbra::Object* m_umbraObject;
	// Umbra bounding object model
	Umbra::Model* m_umbraModel;
	bool m_isDirty;

	// Computed flare intensity
	float m_flareIntensity;
	// Radius of the occluder billboard (in world units)
	float m_occluderSize;
	// Query object for unoccluded billboard
	Tr2OcclusionQueryAL m_totalQuery;
	// Query object for occluded billboard
	Tr2OcclusionQueryAL m_actualQuery;
	// Is the unoccluded query issued?
	bool m_isTotalQueryIssued;
	// Is the occluded query issued?
	bool m_isActualQueryIssued;
	// Number of pixels reported by unoccluded query
	unsigned int m_totalNumOfPixels;
	// Number of pixels reported by occluded query
	unsigned int m_actualNumOfPixels;

	// Vertex declaration for billboard geometry
	unsigned int m_vertexDecl;
	// Vertex buffer for flare/occluder billboard
	Tr2VertexBufferAL m_billboardVB;

	// Occluder material
	Tr2ShaderMaterialPtr m_occluderMaterial;
	// Flare material for material pass
	Tr2ShaderMaterialPtr m_transparentFlareMaterial;
	// Flare material for flares pass
	Tr2ShaderMaterialPtr m_flareMaterial;

	// SH sample index (used for Enlighten SH probes)
	unsigned int m_shSampleIndex;
	// SH matrix (just a stub for Get*LightProbeMatrix functions)
	Matrix m_shMatrix;
	// Does the flare need to update its visibility
	bool m_updateVisibility;
	// Overriden viewport offset;
	TriViewportPtr m_overridenViewport;
	// Hide flare
	bool m_debugIsHidden;

	// Flare shader data for material pass
	PTr2InteriorFlareDataVector m_transparentFlareData;
	// Flare shader data for flare pass
	PTr2InteriorFlareDataVector m_flareData;

	PTriCurveSetVector m_curveSets;

	// Maximum number of flare images per draw call
	static const unsigned int MAX_FLARE_IMAGES;

	class FlareBatch;
	struct FlarePerObjectVSData;

	Tr2ConstantBufferAL	m_occlusionVSConst;
};

TYPEDEF_BLUECLASS( Tr2InteriorFlare );
BLUE_DECLARE_VECTOR( Tr2InteriorFlare );

#endif // Tr2InteriorFlare_H