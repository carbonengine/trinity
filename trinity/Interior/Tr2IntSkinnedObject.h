#ifndef Tr2IntSkinnedObject_h
#define Tr2IntSkinnedObject_h

#include "include/ITr2Interior.h"
#include "Tr2InteriorLightSet.h"
#include "Tr2SkinnedObject.h"

//--------------------------------------------------------------------------------------------------
// forwards
//
class TriFrustum;

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
	public IBluePlacementObserver,
	public ITr2BoundingBox
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

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorCullable
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const;

    //////////////////////////////////////////////////////////////////////////
    // ITr2InteriorDynamic
	virtual bool AddToScene( Tr2ApexScene* apexScene );
	virtual void RemoveFromScene( void );

	// Per-object data with instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet,
		const Matrix& objectToWorldMatrix 
		);

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
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ); 

	//////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t ) { return this->GetRawRoot(); }
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	//////////////////////////////////////////////////////////////////////////
	// ITr2InteriorDynamic
	
	virtual void SetLOD( const TriFrustum* frustum );

	// sizes
	virtual bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;
	virtual bool IsBoundingBoxReady( void ) const;
	
	// Apex
	void AddToApexScene( Tr2ApexScene* apexScene );
	void RemoveFromApexScene( void );

	//////////////////////////////////////////////////////////////////////////
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front_, const Vector3& top_, const Vector3& pos_ );

protected:
	void AddReflectionMap( TriTextureRes* texture );
	void RemoveReflectionMap( TriTextureRes* texture );
protected:
	friend class WodAvatar2Builder;

	// bounding sphere info
	Vector4 m_boundingSphere;

	// lightsources on this avatar
	Tr2InteriorLightSet m_lightSet;

	Vector3 m_currentPosition;
	Vector3 m_currentScaling;
	Quaternion m_currentRotation;
	bool m_positionSet;
	bool m_scalingSet;
	bool m_rotationSet;

	// Apex
	bool m_isInApexScene;

	Tr2PerObjectData* GetPerObjectDataGpuSkinning( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix );

	// Per-cell reflection maps
	TriTextureResPtr m_cellReflectionMaps[2];
	// Time since last per-cell reflection map change
	float m_cellReflectionTime;
	// Time of the previous PostPhysicsUpdate
	Be::Time m_previousUpdateTime;

	// Local variable store for this object
	Tr2VariableStorePtr m_variableStore;

	// Depth offset for transparency sorting
	float m_depthOffset;
};

TYPEDEF_BLUECLASS( Tr2IntSkinnedObject );

#endif // Tr2IntSkinnedObject_h
