#ifndef Tr2IntSkinnedObject_h
#define Tr2IntSkinnedObject_h

#include "include/ITr2Interior.h"
#include "Tr2InteriorLightSet.h"
#include "Tr2SkinnedObject.h"

//--------------------------------------------------------------------------------------------------
// forwards
//
class TriFrustum;
namespace Umbra
{
	class Object;
	class Model;
}

BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( TriTextureRes );

//--------------------------------------------------------------------------------------------------
// Specialization of Tr2SkinnedObject for use in Tr2InteriorScene
//
BLUE_DECLARE( Tr2IntSkinnedObject );
BLUE_DECLARE_VECTOR( Tr2IntSkinnedObject );
BLUE_DECLARE( Tr2ApexScene );

class Tr2IntSkinnedObject : 
	public ITr2InteriorDynamic,
	public IInitialize,
	public Tr2SkinnedObject,
	public ITr2Pickable, 
	public IBluePlacementObserver
{
public:
    EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

    Tr2IntSkinnedObject(IRoot* lockobj = NULL);
    ~Tr2IntSkinnedObject();

	// Update functions for Tr2SkinnedObject (implements the ITr2InteriorDynamic interface)
	//
	virtual void PrePhysicsUpdate( Be::Time time );
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene );
	virtual void SetSHSampleIndex( unsigned int index ) { m_shSampleIndex = index; }
	virtual unsigned int GetSHSampleIndex() const  { return m_shSampleIndex; }
	virtual Matrix& GetRedLightProbeMatrix( void ) { return m_SHMatrixRed; }
	virtual Matrix& GetGreenLightProbeMatrix( void ) { return m_SHMatrixGreen; }
	virtual Matrix& GetBlueLightProbeMatrix( void ) { return m_SHMatrixBlue; }

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorDynamic
	virtual void SetVisibility( bool bVisible );
	virtual bool IsVisible( void ) const { return m_isVisible; }
	virtual bool DoVisualizeLightProbes( void ) const;

	virtual bool AddToScene( Tr2ApexScene* apexScene );
	virtual void RemoveFromScene( void );

	virtual void SetVisibleLightCount( int visibleLightCount );
	virtual void SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet ) { }

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

	// Set mirror depth
	virtual void SetMirrorDepth( int depth ) { m_mirrorDepth = depth; }
	virtual void SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver ) { m_shSolver = solver; }

	virtual bool CastsShadows() const { return m_castsShadows; }

	virtual void SetPosition(const Vector3 &pos);
	virtual void SetRotation( const Quaternion& rotQuat );
	virtual void SetScaling( const Vector3& scaleVec );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData ); 

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID() { return this->GetRawRoot(); }

	//////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	
	// Umbra interaction
	virtual bool IsUmbraReady( void ) const { return (m_umbraModel != NULL); }
	virtual bool TestCellIntersectionAndAdd( Tr2InteriorCell* cell );
	virtual void CellRemoved( Tr2InteriorCell* cell );
	virtual bool IsDirty( void ) const { return m_isDirty; }
	virtual void ClearDirty( void ) { m_isDirty = false; }
	// Set the dirty flag
	void SetDirtyFlag( bool isDirty ) { m_isDirty = isDirty; }
	virtual bool IsBackgroundProxy( void ) const { return false; }
	virtual void AddToCellAsBackgroundProxy( Umbra::Cell* cell ) {}
	virtual void AddToRootCell( Umbra::Cell* cell );
	virtual bool IsShadowCaster( void ) const { return true; }

	virtual void UpdateUmbraObject( Umbra::Cell* cell, Umbra::Object*& object ) const;

	virtual void SetLOD( const TriFrustum* frustum );

	// sizes
	virtual bool GetBoundingSphere( Vector4& sphere ) const;
	virtual bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool IsBoundingBoxReady( void ) const;
	virtual bool GetShProbePosition( Vector3& position ) const;
	
	// Apex
	void AddToApexScene( Tr2ApexScene* apexScene );
	void RemoveFromApexScene( void );

	//////////////////////////////////////////////////////////////////////////
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front_, const Vector3& top_, const Vector3& pos_ );

	void BindLowLevelShaders();

protected:
	virtual bool DoDisplay( void ) const { return m_display || (m_mirrorDepth > 0); }
	virtual void ExplicitBoundsChanged();

	void AddReflectionMap( TriTextureRes* texture );
	void RemoveReflectionMap( TriTextureRes* texture );
protected:
	friend class WodAvatar2Builder;

	// bounding sphere info
	Vector4 m_boundingSphere;

	// lightsources on this avatar
	Tr2InteriorLightSet m_lightSet;

	// index of SH task in Enlighten task manager
	unsigned int m_shSampleIndex;
	// interpolated light probe matrices
	Matrix m_SHMatrixRed;
	Matrix m_SHMatrixGreen;
	Matrix m_SHMatrixBlue;

	// culling
	bool m_isVisible;
	std::vector<Umbra::Object*> m_umbraObjects;
	Umbra::Model* m_umbraModel;
	bool m_isDirty;
	int m_mirrorDepth;

	Vector3 m_currentPosition;
	Vector3 m_currentScaling;
	Quaternion m_currentRotation;
	bool m_positionSet;
	bool m_scalingSet;
	bool m_rotationSet;

	// Number of visible lights
	int m_visibleLightCount;

	// Apex
	bool m_isInApexScene;

	// as long as we support cpu AND gpu skinning we need two different ways of ::GetPerObjectData()
	Tr2PerObjectData* GetPerObjectDataCpuSkinning( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix );
	Tr2PerObjectData* GetPerObjectDataGpuSkinning( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix );

	// as long as we support cpu AND gpu skinning we need two different ways of ::GetPerObjectData()
	Tr2PerObjectData* GetPerObjectDataForPrePassCpuSkinning( 
		ITriRenderBatchAccumulator* accumulator,
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix );
	Tr2PerObjectData* GetPerObjectDataForPrePassGpuSkinning( 
		ITriRenderBatchAccumulator* accumulator,
		const Matrix& objectToWorldMatrix, 
		const Matrix& mirrorToWorldMatrix );

	// Clear all Umbra data
	void ClearUmbra( void );

	// Rebuild bounding volume
	void RebuildVolume( void );

	void UpdateUmbraTransforms( void );
	// SH lighting solver for transparent rendering
	ITr2InteriorSHLightingSolver *m_shSolver;

	// Per-cell reflection maps
	TriTextureResPtr m_cellReflectionMaps[2];
	// Time since last per-cell reflection map change
	float m_cellReflectionTime;
	// Time of the previous PostPhysicsUpdate
	Be::Time m_previousUpdateTime;

	// Local variable store for this object
	Tr2VariableStorePtr m_variableStore;

	// Flag to indicate that this object casts shadows
	bool m_castsShadows;

	// Offset for Enlighten SH probe position (in world space)
	Vector3 m_probeOffset;
	// Depth offset for transparency sorting
	float m_depthOffset;

	// Cached mirror-to-world matrix from last GetPerObjectData call
	Matrix m_mirrorToWorldMatrix;
};

TYPEDEF_BLUECLASS( Tr2IntSkinnedObject );

#endif // Tr2IntSkinnedObject_h
