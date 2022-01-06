#ifndef TR2SKINNEDOBJECT_H
#define TR2SKINNEDOBJECT_H

 
#include "ITr2Renderable.h"
#include "ITr2SkinnedObject.h"
#include "Tr2SkinnedModel.h"
#include "IWorldPosition.h"
#include "Tr2SkinnedObjectLOD.h"
#include "Utilities/BoundingBox.h"
#include "include/TriMatrix.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( Tr2SkinnedObject );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE_INTERFACE( ITr2AnimationUpdater );
BLUE_DECLARE_INTERFACE( ITr2WorldTransformUpdater );

BLUE_DECLARE( Tr2ApexScene );

class TriFrustum;

class Tr2SkinnedObject: 
	public ITr2Renderable,
    public ITr2SkinnedObject,
    public IWorldPosition,
	public IListNotify,
	public INotify,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	using ITr2Renderable::Lock;
	using ITr2Renderable::Unlock;

	Tr2SkinnedObject(IRoot* lockobj = NULL);
	~Tr2SkinnedObject();

	virtual void PrePhysicsUpdate( Be::Time time );
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene );

	// Get the matrices to use for skinning, subject to frame delay to match up with cloth simulation
	float* GetSkinningMatrices();

	// Get the world transform to use for skinning, subject to frame delay to match up with cloth simulation
	const Matrix& GetSkinningTransform() const;

	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	void ResetAnimationBindings();

	const Matrix& GetTransform() const { return *m_transform.GetMatrix(); }

	const Vector3& GetPosition() const { return *(reinterpret_cast<const Vector3*>(&m_transform._41)); }
	virtual void SetPosition(const Vector3 &pos);

	const Quaternion GetRotation() const;
	virtual void SetRotation( const Quaternion &rotQuat );

	const Vector3 GetScaling() const;
	virtual void SetScaling( const Vector3 &scaleVec );
	
	bool GetLocalBoundingBox( Vector3& min, Vector3& max ) const;
	bool GetClippedWorldBoundingObb( const Matrix& localToWorld, Vector3& x, Vector3& y, Vector3& z, Vector3& center, Vector3& sizes, TriFrustum* cameraFrustum );

	unsigned int GetSkinningMatrixFrameDelay() const;
	void SetSkinningMatrixFrameDelay(unsigned int val);

	// lod
	virtual void SetLOD( const TriFrustum* frustum );
	void SetHighDetailModel( Tr2SkinnedModel* model );
	void SetMediumDetailModel( Tr2SkinnedModel* model );
	void SetLowDetailModel( Tr2SkinnedModel* model );
	int  GetCurrentLod() const { return m_lod.GetCurrentLod(); }

	// for lod, used to estimate projected pixel size and hence lod.
	virtual bool GetBoundingSphere( Vector4& /*sphere*/ ) const { return false; }
	virtual bool GetWorldBoundingBox( Vector3& /*min*/, Vector3& /*max*/ ) const { return false; }

	//////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	virtual Vector3 GetWorldPosition() { return GetPosition(); }
	virtual Quaternion GetWorldRotation() { return GetRotation(); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ); 

	virtual bool HasTransparentBatches();
	virtual float GetSortValue() { return 0.0f; }; 

	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator ) { return NULL; };

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2,	IRoot* value, const IList* theList );

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );
	
	// find the bone with this name in the animation rig
	virtual unsigned GetBoneIndex( const std::string & boneName ) const;
	virtual const Matrix * GetBoneTransform( unsigned joint ) const;
	/// \return a number that increases anytime the bone bindings get invalidated (ie due to lod)
	virtual unsigned GetSkeletonTag() const;
	/// simpler version for python use
	virtual Vector3 GetBonePosition( unsigned joint ) const;

	// for debugging/inspection: send all bones to LOGRELEASE
	void PrintAllBones();

protected:
	void AllocateSkinningMatrices( unsigned int numBones );
	void FreeSkinningMatrices();
	void ResetSkinningMatrices();
	void UpdateBones( Be::Time time, Tr2ApexScene* apexScene );
	void UpdatePerObjectData();

protected:
    ITr2AnimationUpdaterPtr m_animationUpdater;
    ITr2WorldTransformUpdaterPtr m_worldTransformUpdater;

    void ClearBoneInfo();

	AxisAlignedBoundingBox GetBoundingBoxInLocalSpace() const;

	// Utility function to determine whether to accumulate render batches for this object
	virtual bool DoDisplay( void ) const { return m_display; }

	std::string m_name;

	// Transform that simply represents position, scale and rotation
	// as a 4x4 matrix.
    PTriMatrix m_transform;

	Tr2SkinnedModelPtr m_visualModel;
	unsigned	m_skeletonTag;

	// Cloth simulation is based off the skinning matrices
	// but can introduce a frame delay of 1-3 frames. To
	// compensate we allow the non-simulated pieces to
	// be delayed to match up with the simulated parts.
	// Each matrix in the queue is a 3x4 matrix, extracted
	// from the animation updater (ITr2AnimationUpdater)
	// 4x4 matrices.
	typedef TrackableStdVector<float*> SkinningMatrixQueue_t;
	typedef TrackableStdVector<Matrix*> AccumulatedTransforms_t;
	typedef TrackableStdVector<Matrix> WorldTransforms_t;
	
	// Queue of matrices used for skinning
	SkinningMatrixQueue_t m_skinningMatrixQueue;

	// World transforms that go with the skinning matrices
	WorldTransforms_t m_worldTransformsQueue;

	// Accumulated transforms, used for debug rendering (skeleton trail)
	AccumulatedTransforms_t m_accumulatedTransformsQueue;

	unsigned int m_skinningMatrixFrameDelay;
	unsigned int m_skinningMatrixQueueIndex;

	const std::string* m_boneList;
	unsigned int m_skinningMatrixCount;

	unsigned int* m_animRigToRenderRigMapping;
	unsigned int* m_renderRigToAnimRigMapping;

    TrackableStdVector<std::string> m_renderRigBoneList;
    unsigned int m_numRenderRigBones;

	// This is set when the frame queue is initialized. In the first
	// animation update the queue is then primed with copies of the
	// matrices from the animation, rather than leaving garbage
	// (or identity matrices) in the queue.
	bool m_skinningMatrixQueueNeedsPriming;

	bool m_display;

	Vector3 m_lastTranslation;

	// LOD selection
	Tr2SkinnedObjectLod	m_lod;
	float m_estimatedPixelDiameter;
	// remember the time from the last Update call
	Be::Time m_lastUpdateTime;

	PTriCurveSetVector m_curveSets;

	bool m_useDynamicBounds;
	bool m_useExplicitBounds;
	bool m_hasDynamicBounds;

	Vector3 m_minDynamicBounds;
	Vector3 m_maxDynamicBounds;

	Vector3 m_minBounds;
	Vector3 m_maxBounds;

	float		m_updatePeriod;
};

TYPEDEF_BLUECLASS( Tr2SkinnedObject );
BLUE_DECLARE_VECTOR( Tr2SkinnedObject );

#endif
