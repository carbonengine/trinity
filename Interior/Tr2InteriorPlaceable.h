#pragma once
#ifndef Tr2InteriorPlaceable_H
#define Tr2InteriorPlaceable_H

#include "include/ITr2Interior.h"
#include "include/TriMatrix.h"
#include "ITr2Renderable.h"
#include "Tr2InteriorLightSet.h"
#include "Tr2InteriorRenderBatch.h"
#include "Utilities/BoundingBox.h"

//--------------------------------------------------------------------------------------------------
// Forward declarations
//
class TriFrustum;
class Tr2InteriorMirror;
class Tr2LitPerObjectData;

namespace Umbra
{
	class Object;
	class Model;
}

//--------------------------------------------------------------------------------------------------
// Blue declarations
//
BLUE_DECLARE( Tr2InteriorPlaceable );
BLUE_DECLARE( WodPlaceableRes );
BLUE_DECLARE_VECTOR( Tr2InteriorPlaceable );

BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2VariableStore );

// -----------------------------------------------------------------------------------------------------
// An interior placeable object.
// Description
//   Interior placeable objects are instanced geometric objects.
//   They are dynamically illuminated by spherical harmonics light
//   probes. Interior placeables are managed by Tr2InteriorScene,
//   but inhabit one or more Tr2InteriorCells, as determined by a
//   bounding box intersection test.
// See Also
//   Tr2InteriorStatic, Tr2IntSkinnedObject, Tr2InteriorScene     
// -----------------------------------------------------------------------------------------------------
class Tr2InteriorPlaceable : 
    public INotify,
    public IInitialize,
    public ITr2Renderable,
	public ITr2Pickable,
	public ITr2InteriorDynamic,
	public ITr2BoundingBox
{
public:
    EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	// Constructor/destructor
	Tr2InteriorPlaceable( IRoot* lockobj = NULL );
    ~Tr2InteriorPlaceable();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
    virtual bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
    // IInitialize
    bool Initialize( void );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	virtual void SetVisibility( bool bVisible );
	virtual bool IsVisible( void ) const { return m_isVisible; }
	virtual bool DoVisualizeLightProbes( void ) const;
	virtual bool AddToScene( Tr2ApexScene *apexScene );
	virtual void RemoveFromScene( void );
	virtual bool GetBoundingSphere( Vector4& sphere ) const;
	virtual bool GetLocalBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool IsBoundingBoxReady( void ) const;
	virtual bool GetShProbePosition( Vector3& position ) const;
	virtual void PrePhysicsUpdate( Be::Time time );
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene );
	virtual void SetSHSampleIndex( unsigned int index ) { m_shSampleIndex = index; }
	virtual unsigned int GetSHSampleIndex() const  { return m_shSampleIndex; }
	virtual Matrix& GetRedLightProbeMatrix( void ) { return m_SHMatrixRed; }
	virtual Matrix& GetGreenLightProbeMatrix( void ) { return m_SHMatrixGreen; }
	virtual Matrix& GetBlueLightProbeMatrix( void ) { return m_SHMatrixBlue; }
	virtual bool CastsShadows() const { return true; }

	// Umbra interaction
	virtual bool IsUmbraReady( void ) const { return (m_umbraModel != NULL); }
	virtual bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell );
	virtual void CellRemoved( Tr2InteriorCell* cell );
	virtual bool IsDirty( void ) const { return m_isDirty; }
	virtual void ClearDirty( void );
	// Set the dirty flag
	void SetDirtyFlag( bool isDirty ) { m_isDirty = isDirty; }
	void MarkAsDirty() { m_isDirty = true; }
	virtual bool IsBackgroundProxy( void ) const;
	virtual void AddToCellAsBackgroundProxy( Umbra::Cell* cell );
	virtual void AddToRootCell( Umbra::Cell* cell );
	virtual bool IsShadowCaster( void ) const;
	virtual bool IsStatic( void ) const;

	virtual void UpdateUmbraObject( Umbra::Cell* cell, Umbra::Object*& object ) const;

	virtual void SetLOD( const TriFrustum* frustum );

	virtual void SetVisibleLightCount( int visibleLightCount );

	void BoundingBoxReset(); 
	void BoundingBoxOverride( Vector3& min, Vector3& max );

	// debug
	void RenderDebugInfo( TriLineSetPtr lines ) const;
	virtual void SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, 
							 TriBatchType batchType,
							 const Tr2PerObjectData* data ); 

	virtual bool HasTransparentBatches( void );
    virtual float GetSortValue( void );

	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	// Per-object data with instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet,
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix 
	);

	// Per-object data for pre-pass
	virtual Tr2PerObjectData* GetPerObjectDataForPrePass(
		ITriRenderBatchAccumulator* accumulator,
		const Matrix& objectToWorldMatrix
	);

	virtual void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver ) { m_shSolver = solver; }
	virtual const ITr2RenderableVector* GetAttachedRenderables();

	// Set stencil parameters
	void SetStencilParameters( const WodStencilBatchParams& params )
	{
		m_stencilParams = params;
	}

	// Set mirror depth
	virtual void SetMirrorDepth( int depth ) {}

	// Get mirror by index
	Tr2InteriorMirror* GetMirror( size_t index ) const;

	void EnableMirrorPortals( bool enable );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( void ) { return this->GetRawRoot(); }

	/////////////////////////////////////////////////////////////////////////////////////

	std::string GetPlaceableResPath( void ) const;
    void SetPlaceableResPath( const std::string& val );
	// Set the placeableRes from the placeableResPath by loading it from disk
	void LoadPlaceableRes();

    const Matrix& GetTransform( void ) const { return m_transform; }

	// Position getter & setter
	const Vector3& GetPosition( void ) const { return *(reinterpret_cast<const Vector3*>(&m_transform._41)); }
	void SetPosition( const Vector3& pos );

	// Rotation getter & setter
	const Quaternion GetRotation( void ) const;
	void SetRotation( const Quaternion& rotQuat );

	// Scaling getter & setter
	const Vector3 GetScaling( void ) const;
	void SetScaling( const Vector3& scaleVec );

	void BindLowLevelShaders();

protected:
	Tr2PerObjectData* GetPerObjectDataWithLightSet( ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, const Matrix& objectToWorldMatrix, const Matrix& mirrorToWorldMatrix );

private:
	float CalculateCameraDistance( void );
	
	// Clear all Umbra data (except mirrors)
	void ClearUmbra( void );

	// Clear mirrors
	void ClearMirrors( void );

	// Rebuild bounding volume
	void RebuildVolume( void );

	// Calculate bounding box
	void CalculateBoundingBox( Vector3& min, Vector3& max );

	void UpdateUmbraTransforms( void );

	// Mirrors
	void EnableMirrors( Umbra::Cell* cell );
	void DisableMirrors( void );

	void AddReflectionMap( TriTextureRes* texture );
	void RemoveReflectionMap( TriTextureRes* texture );
private:
	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;
	AxisAlignedBoundingBox GetBoundingBoxInWorldSpace() const;

	std::string m_name;

	// Umbra object
	bool m_isVisible;
	std::vector<Umbra::Object*> m_umbraObjects;
	Umbra::Model* m_umbraModel;

	bool m_display;

	// When false, the placeableRes loaded is a shared instance
	// When true, the placeableRes is a unique copy that can be modified without 
	// effects on other placables loaded from the same path
	bool m_isUniqueInstance;

	Vector3 m_currentPosition;
	Vector3 m_currentScaling;
	Quaternion m_currentRotation;
	bool m_positionSet;
	bool m_scalingSet;
	bool m_rotationSet;
	PTriMatrix m_transform;

	// Dirty flag, for signaling Umbra update
	bool m_isDirty;

	// True when a placeable is not intended to move, animate or be added and removed from the scene
	// used to allow placeables cast 'static' shadows 
	bool m_isStatic;

	std::string m_placeableResPath;
	WodPlaceableResPtr m_placeableRes;

	// Parameters for stencil batches
	WodStencilBatchParams m_stencilParams;

	Tr2InteriorLightSet m_lightSet;
	bool m_visualizeLightProbes;

	enum
	{
		VISIBILITYMODE_NORMAL,
		VISIBILITYMODE_FADING,
		VISIBILITYMODE_HIDDEN,
	} m_visibilityMode;

	// index of SH task in Enlighten task manager
	unsigned int m_shSampleIndex;
	// interpolated light probe matrices
	Matrix m_SHMatrixRed;
	Matrix m_SHMatrixGreen;
	Matrix m_SHMatrixBlue;

	// Bounding sphere
	Vector4 m_boundingSphere;

	// Umbra mirrors
	std::vector<Tr2InteriorMirror*> m_mirrors;

	// Number of lights affecting this object
	int m_visibleLightCount;

	// Draw light spider for debugging
	bool m_drawLightSpider;
	std::vector<Vector3> m_visibleLights;

	// Vector of attached placables
	PITr2RenderableVector m_attachedObjects;
	// SH lighting solver for transparent rendering
	ITr2InteriorSHLightingSolver *m_shSolver;

	// Bounding box overrides
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	float m_isBoundingBoxModified;

	// Per-cell reflection maps
	TriTextureResPtr m_cellReflectionMaps[2];
	// Time since last per-cell reflection map change
	float m_cellReflectionTime;
	// Is reflection transition finieshed (for placeable shaders with 1 per-cell map)
	bool m_transitionFinished;
	// Time of the previous PostPhysicsUpdate
	Be::Time m_previousUpdateTime;

	// Local variable store for this object
	Tr2VariableStorePtr m_variableStore;

	// Offset for Enlighten SH probe position (in world space)
	Vector3 m_probeOffset;

	// Depth offset for transparency sorting
	float m_depthOffset;

	// Cached mirror-to-world matrix from last GetPerObjectData call
	Matrix m_mirrorToWorldMatrix;
};

TYPEDEF_BLUECLASS( Tr2InteriorPlaceable );

BLUE_DECLARE_VECTOR( Tr2InteriorPlaceable );
TYPEDEF_BLUECLASS( Tr2InteriorPlaceableVector );

#endif
