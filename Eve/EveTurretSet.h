////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#pragma once
#ifndef EveTurretSet_H
#define EveTurretSet_H

#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "include/ITriTargetable.h"
#include "Tr2ShLightingManager.h"

// needed for override
#include "Tr2PerObjectData.h"

// forwards
class TriFrustum;
class TriFrustumOrtho;
struct ITr2Renderable;
struct ViewDistanceInfo;

BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( EveTurretFiringFX );

// constants
// maximum number of single turrets per turret set
const unsigned int EVE_MAX_TURRETS_PER_SET = 24;
// maximum number of bones in all of the turret set
const unsigned int EVE_MAX_TURRET_SET_BONES = EVE_MAX_TURRETS_PER_SET * 3;
// maximum time offset for turret firing
const float EVE_TURRET_RANDOM_DELAY_MAX = 0.6f;

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for turrets
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveTurretSetPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	// vs per object data
	Vector4 m_baseCutoffData;
	Vector4 m_turretSetData;
	Matrix m_shipMatrix;

	// per turret data
	Vector4 m_turretTranslation[EVE_MAX_TURRETS_PER_SET];
	Quaternion m_turretRotation[EVE_MAX_TURRETS_PER_SET];

	// pose information
	Vector4 m_turretPoseTrans[EVE_MAX_TURRET_SET_BONES];
	Quaternion m_turretPoseRot[EVE_MAX_TURRET_SET_BONES];

	// pixel shader per object data
	Vector4 m_shipData;
	Vector4 m_clipData1;
	Vector4 m_clipData2;
	Vector4 m_shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT];
};

// --------------------------------------------------------------------------------
// Description:
//   This class hold a set of turrets. Turrets are always kept in sets, since we
//   have multiple turrets on a ship to deal with the firing arc problem. So the
//   same turret is located many times on a ship, each one having it's own
//   tracking and animation state. To save DIP calls, a turret set is rendering
//   using instancing.
// SeeAlso:
//   EveShip2
// --------------------------------------------------------------------------------
class EveTurretSet :
	public IInitialize,
	public INotify,
	public ITr2GeometryProvider,
	public IBlueAsyncResNotifyTarget,
	public Tr2DeviceResource,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Unlock;
	using IInitialize::Lock;

	EveTurretSet(IRoot* lockobj = NULL);
	~EveTurretSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// public structs
	struct ParentData
	{
		Matrix transform;
		Vector4 shipData;
		Vector4 clipData;
		Vector4 clipDataEx;
	};

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p );
	virtual void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:
	// set local position
	void SetLocalTransform( unsigned int turretIndex, const Matrix* localMatrix );
	// timing and worldspace positioning
	void UpdateSyncronous( float deltaT, Be::Time time, const Matrix* parentMatrix );
	void UpdateAsyncronous( float deltaT, Be::Time time, const ParentData* parentData );

	void SetEffectEndPoint();

	// rendering
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Vector4* shLighting );
	void GetRenderablesCastingShadow( const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables );
	// just debug info
	void RenderDebugInfo( Tr2RenderContext& renderContext );
	// rebuild the bounding sphere size
	void RebuildBoundingSphere();
	// disable LODing
	void FreezeHighDetailLOD();
	// update view distance info
	void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;
	// get the shader
	const Tr2Effect* GetShader() const;
	Tr2Effect* GetShader();

	// action
	void EnterStateDeactive();
	void EnterStateIdle();
	void EnterStateTargeting();
	void EnterStateFiring();
	void EnterStateReloading();

	void ForceStateDeactive();
	void ForceIdleAnimation();

	// get locator names
	const char* GetLocatorName() const;
	const int GetSlotNumber() const;

	// get worldmatrix of the currently firing bone
	Matrix GetFiringBoneWorldTransform( unsigned int muzzle ) const;

	// missed shots
	bool GetShotMissed() const;
	void SetShotMissed( const bool missed );
	double GetLastShotTime() const { return m_lastShotTime; }
	float GetShotTimeVariance() const { return EVE_TURRET_RANDOM_DELAY_MAX; }

	// turret LOD
	enum LOD
	{
		LOD_INVALID = 0,
		LOD_EMPTY,
		LOD_HIGHEST,

		LOD_DISABLED,
	};

private:
	void PopShotMissed();
	void ResetMissQueue();
	void UpdateMissPosition(const Matrix *);
	size_t MissQueueSize() const;

	// turret set states
	enum State
	{
		STATE_INVALID = 0,
		STATE_DEACTIVE,
		STATE_IDLE,
		STATE_TARGETING,
		STATE_FIRING,
		STATE_RELOADING,
	};
	// system-controlled bones
	enum SystemBones
	{
		SYSBONE_INVALID = 0,
		SYSBONE_ROTATION,
		SYSBONE_COUNTER_ROTATION,
		SYSBONE_PITCH,
		SYSBONE_PITCH1,
		SYSBONE_SCALED_HEIGHT,
		SYSBONE_SCALED_PITCH01,
		SYSBONE_SCALED_PITCH02,
		SYSBONE_MAX,
	};
	// accuracy of shot
	enum ShotAccuracy
	{
		ACCURACY_HIT = 0,
		ACCURACY_MISS = 1,
		ACCURACY_INDETERMINATE = 2
	};


	// geom res load
	void InitializeGeometryResource();
	// instance vertex buffer
	void InitializeInstanceBuffer();
	// setup the attached firing effect
	void InitializeFiringEffect();
	// cleanup
	void Cleanup();
	// determine LOD and check for change
	bool UpdateLOD();
	// set transform for tracking
	void ModifySystemBoneTransform( SystemBones bone, const Vector3* target, granny_transform* transform ) const;
	// animation
	float PlayAnimation( unsigned int turretIndex, const std::string& animName, const std::string& animNameIdle, float delay );
	void StopAnimation( unsigned int turretIndex, float delay );
	std::string GetFireAnimationName() const;
	// request a random delay to make turrets on ships look more alive
	void CalcRandomDelay();
	// calc best suited turret and target's locator
	bool GetClosestTurretAndLocator( unsigned int& closestTurretIx, int& closestLocatorIx ) const;

	// name
	std::string m_name;
	// toggle display
	bool m_display;
	bool m_displayEffects;
	bool m_isOnline;
	// how many turrets can actually be displayed, due to bone count
	unsigned int m_possibleTurretDisplayAmount;
	// how many turrets visible in this frame?
	unsigned int m_visibleCount;
	// what is the highest onscreen size?
	float m_estimatedPixelDiameter;
	// current lod level
	LOD m_lodLevel;

	// locator on the parent ship
	std::string m_locatorName;
	int m_slotNumber;

	// parent ship data
	ParentData m_parentData;
	const Vector4* m_parentShLighting;

	// keep a vector of data on each pair of the turret
	struct SingleTurretData
	{
		bool					valid;
		bool					visible;
		// positions
		Vector4					localPosition;
		Quaternion				localQuaternion;
		Matrix					worldMatrix;
		Matrix					invWorldMatrix;

		// animation
		granny_skeleton*		grnSkeleton;
		granny_model_instance*	grnModelInstance;
		granny_local_pose*		grnLocalPose;
		granny_world_pose*		grnWorldPose;
	};
	std::vector<SingleTurretData> m_singleTurrets;

	// bounding sphere
	Vector4 m_boundingSphere;
	// clipping plane
	float m_bottomClipHeight;
	// need special vertex decl, turrets are rendered multi stream
	unsigned int m_vertexDeclHandle;
	// turret draw shader needs to be changable
	Tr2EffectPtr m_turretEffect;
	// shader to render the 2d sprite
	Tr2EffectPtr m_shadowEffect;
	// turret geometry
	unsigned int m_turretVertexDeclElementCount;
	Tr2VertexDefinition m_turretVertexDecl;
	std::string m_geomResPath;
	TriGeometryResPtr m_geometryResource;
	// instance vertex stream
	Tr2VertexBufferAL m_instanceBuffer;

	// target (object we are tracking)
	ITriTargetablePtr m_targetObject;
	int m_targetLocator;
	Vector3 m_targetPosition;

	// miss-related state
	Vector3 m_targetPositionMiss;
	TrackableStdDeque<bool> m_missQueue;
	mutable ShotAccuracy m_lastShotAccuracy;
	double m_lastShotTime;
	bool m_laserMissBehaviour, m_projectileMissBehaviour;
	bool m_readyToFireEffect;
	bool m_trackMissPoint;
	float m_randomMissDistanceOffset;
	Vector3 m_randomMissPositionOffset;

	// tracking
	float m_trackingInfluence;
	float m_trackingInfluenceDelta;
	float m_delayToFadeOutTracking;
	float m_delayToFadeInTracking;
	float m_trackingFadeTime;

	// smooth fading
	Vector3 m_targetPositionOld;
	float m_targetPositionOldInfluence;

	// animation
	struct AnimationRequest
	{
		int						turretIndex;
		std::string				animName;
		std::string				animNameIdle;
	};
	std::vector<AnimationRequest> m_animationQueue;
	granny_model* m_grnModel;
	granny_mesh_binding* m_grnMeshBinding;

	// firing animation specialties
	bool m_useRandomFiringDelay;
	float m_randomFiringDelay;
	unsigned int m_alternateFiringAnimCount;
	bool m_hasCyclingFiringPos;
	unsigned int m_currentCyclingFiresPos;

	// system bones
	unsigned int m_systemBoneID[SYSBONE_MAX];
	// specific system bone values
	float m_sysBoneHeight;
	float m_sysBonePitchOffset;
	float m_sysBonePitchFactor;
	float m_sysBonePitchMin;
	float m_sysBonePitch01Offset;
	float m_sysBonePitch01Factor;
	float m_sysBonePitch02Offset;
	float m_sysBonePitch02Factor;

	// state of turret set
	State m_state;
    // the turret that's currently active(firing)
    int m_activeTurret;

	// time to re-check active turret
	float m_recheckTimeLeft;

	// firing effect redfile path
	std::string m_firingEffectResPath;
	// firing effect
	EveTurretFiringFXPtr m_firingEffect;
	bool m_firingEffectMuzzlePosSet;
};

TYPEDEF_BLUECLASS( EveTurretSet );
BLUE_DECLARE_VECTOR( EveTurretSet );

#endif // EveTurretSet_H
