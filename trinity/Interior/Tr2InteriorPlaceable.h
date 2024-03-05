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

//--------------------------------------------------------------------------------------------------
// Blue declarations
//
BLUE_DECLARE( Tr2InteriorPlaceable );
BLUE_DECLARE( WodPlaceableRes );
BLUE_DECLARE_VECTOR( Tr2InteriorPlaceable );

BLUE_DECLARE( Tr2MeshArea );
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

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	virtual bool AddToScene( Tr2ApexScene *apexScene );
	virtual void RemoveFromScene( void );
	virtual bool GetLocalBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool IsBoundingBoxReady( void ) const;
	virtual void PrePhysicsUpdate( Be::Time time );
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene );

	virtual void SetLOD( const TriFrustum* frustum );

	void BoundingBoxReset(); 
	void BoundingBoxOverride( Vector3& min, Vector3& max );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, 
							 TriBatchType batchType,
							 const Tr2PerObjectData* data,
							 Tr2RenderReason reason = TR2RENDERREASON_NORMAL ); 

	virtual bool HasTransparentBatches( void );
    virtual float GetSortValue( void );

	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	// Per-object data with instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet,
		const Matrix& objectToWorldMatrix
	);

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t ) { return this->GetRawRoot(); }
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

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

protected:
	Tr2PerObjectData* GetPerObjectDataWithLightSet( ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, const Matrix& objectToWorldMatrix );

private:
	float CalculateCameraDistance( void );

	// Rebuild bounding volume
	void RebuildVolume( void );

	// Calculate bounding box
	void CalculateBoundingBox( Vector3& min, Vector3& max );

	void AddReflectionMap( TriTextureRes* texture );
	void RemoveReflectionMap( TriTextureRes* texture );
private:
	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;
	AxisAlignedBoundingBox GetBoundingBoxInWorldSpace() const;

	std::string m_name;

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

	std::string m_placeableResPath;
	WodPlaceableResPtr m_placeableRes;

	Tr2InteriorLightSet m_lightSet;

	// Bounding sphere
	Vector4 m_boundingSphere;

	// Bounding box overrides
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
	bool m_isBoundingBoxModified;

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
};

TYPEDEF_BLUECLASS( Tr2InteriorPlaceable );

BLUE_DECLARE_VECTOR( Tr2InteriorPlaceable );
TYPEDEF_BLUECLASS( Tr2InteriorPlaceableVector );

#endif
