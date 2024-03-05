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
#include "Tr2GrannyAnimation.h"
#include "EveTurretTarget.h"
#include "Tr2DebugRenderer.h"

#include "Controllers/ITr2ControllerOwner.h"
#include "Eve/SpaceObject/Children/EveChildInstanceContainer.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"

// needed for override
#include "Tr2PerObjectData.h"
#include "Shader/IShaderConfigurer.h"

// forwards
class TriFrustum;
class TriFrustumOrtho;
struct ITr2Renderable;
struct ViewDistanceInfo;
class EveUpdateContext;
class Tr2LightManager;

BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( EveTurretFiringFX );
BLUE_DECLARE( EveTurretTarget );
BLUE_DECLARE( Tr2DebugRenderer );
BLUE_DECLARE( Tr2QuadRenderer );
BLUE_DECLARE( TriObserverLocal );

// constants
// maximum number of single turrets per turret set
const unsigned int EVE_MAX_TURRETS_PER_SET = 24;
// maximum number of bones in all of the turret set
const unsigned int EVE_MAX_TURRET_SET_BONES = EVE_MAX_TURRETS_PER_SET * 3;
// maximum time offset for turret firing
const float EVE_TURRET_RANDOM_DELAY_MAX = 0.6f;

struct EveTurretSetVSData {
    Vector4 m_baseCutoffData;
    Vector4 m_turretSetData;
    Matrix m_shipMatrix;

    // per turret data
    Vector4 m_turretTranslation[EVE_MAX_TURRETS_PER_SET];
    Quaternion m_turretRotation[EVE_MAX_TURRETS_PER_SET];

    // pose information
    // For each turret we have a position (Vector4) and then quaternion (and then position and then quaternion etc.)
    float m_turretPosAndRotationBuffer[4 * 2 * EVE_MAX_TURRET_SET_BONES];
};

struct EveTurretSetPSData {
    Vector4 m_shipData;
    Vector4 m_clipData1;
    Vector4 m_shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT];
};

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for turrets
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveTurretSetPerObjectData : public Tr2PerObjectData
{
public:
	void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const override;

	// vs per object data
    EveTurretSetVSData m_vsData;

	// pixel shader per object data
    EveTurretSetPSData m_psData;
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
BLUE_CLASS( EveTurretSet ):
	public IInitialize,
	public INotify,
	public ITr2GeometryProvider,
	public IBlueAsyncResNotifyTarget,
	public Tr2DeviceResource,
	public ITr2Renderable,
	public IShaderConfigurer,
	public ITr2ControllerOwner,
	public EveEntity,
	public ITr2DebugRenderable,
	public IEveShadowCaster
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Unlock;
	using IInitialize::Lock;

	explicit EveTurretSet(IRoot* lockobj = nullptr);
	~EveTurretSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value ) override;
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches() override;
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p ) override;
	void RebuildCachedData( BlueAsyncRes* p ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name );
	void StartControllers();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;
	
	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	bool IsCastingShadow( const TriFrustum& cameraFrustum, const TriFrustumOrtho& shadowFrustum, const uint32_t shadowMapSize, const Vector3 sunDir, float& sizeInShadow ) const override;
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) override;
	Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator ) override;

	int GetState() const;

private:
	bool OnPrepareResources() override;

public:
	// set local position
	void SetLocalTransform( unsigned int turretIndex, const Matrix* localMatrix );
	void UpdateTurretTransforms(const Matrix* parentWorldMatrix);

	// timing and worldspace positioning
	void UpdateSyncronous( EveUpdateContext& updateContext, const Matrix* parentMatrix );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const IEveSpaceObject2::ParentData* parentData );

	// rendering
	void UpdateVisibility( const TriFrustum& frustum );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, const Vector4* shLighting );
	
	// rebuild the bounding sphere size
	void RebuildBoundingSphere();
	// disable LODing
	void FreezeHighDetailLOD();
	// get the shader
	const Tr2Effect* GetShader() const;
	Tr2Effect* GetShader();

	// action
	void EnterStateDeactive();
	void EnterStateIdle();
	void EnterStateTargeting();
	void EnterStateFiring();
	bool SetupFiringState();
	void EnterStateReloading();

	void ForceStateDeactive();
	void ForceIdleAnimation();
	void ForceStateTargeting();


	// get locator names
	const char* GetLocatorName() const;
	int GetSlotNumber() const;
	unsigned int GetSwarmID() const { return m_swarmID; }

	bool GetLocalBoundingBox( Vector3& aabbMin, Vector3& aabbMax );

	// get worldmatrix of the currently firing bone
	Matrix GetFiringBoneWorldTransform( unsigned int muzzle ) const;

	// missed shots
	void SetShotMissed( bool missed );
	double GetLastShotTime() const;
	float GetShotTimeVariance() const { return EVE_TURRET_RANDOM_DELAY_MAX; }
	size_t MissQueueSize() const;

	void GetLights( Tr2LightManager& lightManager ) const;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	   
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	// turret LOD
	enum LOD
	{
		LOD_INVALID = 0,
		LOD_EMPTY,
		LOD_HIGHEST,

		LOD_DISABLED,
	};

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
private:
	
	// system-controlled bones
	enum SystemBones
	{
		SYSBONE_INVALID = 0,
		SYSBONE_ROTATION,
		SYSBONE_ROTATION01,
		SYSBONE_ROTATION02,
		SYSBONE_COUNTER_ROTATION,
		SYSBONE_PITCH,
		SYSBONE_PITCH1,
		SYSBONE_PITCH2,
		SYSBONE_SCALED_HEIGHT,
		SYSBONE_SCALED_PITCH01,
		SYSBONE_SCALED_PITCH02,
		SYSBONE_SCALED_PITCH03,
		SYSBONE_SCALED_PITCH04,
		SYSBONE_SCALED_PITCH05,
		SYSBONE_SCALED_PITCH06,
		SYSBONE_MAX,
	};

	// geom res load
	void InitializeGeometryResource();
	// instance vertex buffer
	void InitializeInstanceBuffer();
	// setup the attached firing effect
	void InitializeFiringEffect();
	// setup the attached ambient effect
	void InitializeAmbientEffect();
	IEveSpaceObjectChild* GetAmbientEffect() const;
	void SetAmbientEffectControllerVariableOnInstance( int index, const char* name, float value );
	bool IsAmbientVisible() const;

	// cleanup
	void Cleanup();
	// determine LOD and check for change
	bool UpdateLOD();
	// set transform for tracking
	void ModifySystemBoneTransform( SystemBones bone, const Vector3* target, granny_transform* transform, const Matrix* localTransform ) const;

	// Calculates the pitch for a bone based on the parameters
	void CalcTransformForPitchBone( const Vector3* target, granny_transform* transform, float minPitch, float maxPitch, unsigned int boneIndex, const Matrix* localTransform  ) const;
	
	// Returns the correct pitch factor for a specific bone index
	float GetBonePitchFactor(unsigned int boneIndex) const;
	// Returns the correct pitch offset for a specific bone index
	float GetBonePitchOffset(unsigned int boneIndex) const;

	// animation
	float PlayAnimation( unsigned int turretIndex, const std::string& animName, const std::string& animNameIdle, float delay = 0.f );
	void StopAnimation( unsigned int turretIndex, float delay = 0.f );
	std::string GetFireAnimationName() const;
	// request a random delay to make turrets on ships look more alive
	void CalcRandomDelay();
	// calc best suited turret and target's locator
	bool GetClosestTurretAndLocator( unsigned int& closestTurretIx, int& closestLocatorIx ) const;

	void SetTurretBonePose( EveTurretSetPerObjectData* perObjectData, int boneIndex, const Vector3& poseTranslation, const Quaternion& poseRotation );

	// name
	std::string m_name;
	// toggle display
	bool m_display;
	bool m_displayEffects;
	bool m_isOnline;
	bool m_updatePitchPose;
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

	// used by fighters
	unsigned int m_swarmID;

	// parent ship data
	IEveSpaceObject2::ParentData m_parentData;
	const Vector4* m_parentShLighting;

	// keep a vector of data on each pair of the turret
	struct SingleTurretData
	{
		bool					valid;
		bool					visible;
		// positions
		Vector4					localPosition;
		Quaternion				localQuaternion;
		Matrix					localMatrix;
		Matrix					worldMatrix;

		// animation
		granny_skeleton*		grnSkeleton;
		granny_model_instance*	grnModelInstance;
		granny_local_pose*		grnLocalPose;
		granny_world_pose*		grnWorldPose;
	};
	std::vector<SingleTurretData> m_singleTurrets;
	std::vector<GrannyBoneBindingBounds> m_boneBounds;
	bool m_useDynamicBounds;
	
	void InitializeDynamicBounds( granny_file_info* fi, granny_skeleton* skeleton );
	bool GetDynamicBounds( const SingleTurretData& turret, Vector4* boundingSphere, Vector3* aabbMin, Vector3* aabbMax ) const;
	void RenderDynamicBounds();

	// bounding sphere
	Vector4 m_boundingSphere;
	// clipping plane
	float m_bottomClipHeight;
	// need special vertex decl, turrets are rendered multi stream
	unsigned int m_vertexDeclHandle;
	// turret draw shader needs to be changable
	Tr2EffectPtr m_turretEffect;
	// turret geometry
	unsigned int m_turretVertexDeclElementCount;
	Tr2VertexDefinition m_turretVertexDecl;
	std::string m_geomResPath;
	TriGeometryResPtr m_geometryResource;
	// instance vertex stream
	Tr2BufferAL m_instanceBuffer;

	// Assign the target object
	void SetTargetObject( IRoot* target );
	ITriTargetablePtr GetTargetObject();
	void SetTargetScale();

	// target (object we are tracking)
	EveTurretTargetPtr m_target;

	// miss-related state
	bool m_laserMissBehaviour, m_projectileMissBehaviour;

	// impacts
	float m_impactSize;
	ImpactBehaviour::Type m_impactBehaviour;

	// tracking
	float m_trackingInfluence;
	float m_trackingInfluenceDelta;
	float m_delayToFadeOutTracking;
	float m_delayToFadeInTracking;
	float m_maxTrackingTime;

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
	uint32_t m_maxCyclingFirePos;
	uint32_t m_cyclingFireGroupCount;
	uint32_t m_currentCyclingFiresPos;

	// system bones
	unsigned int m_systemBoneID[SYSBONE_MAX];
	// specific system bone values
	float m_sysBoneHeight;
	float m_sysBonePitchOffset;
	float m_sysBonePitchFactor;
	float m_sysBonePitchMin;
	float m_sysBonePitchMax;
	float m_sysBonePitch01Offset;
	float m_sysBonePitch01Factor;
	float m_sysBonePitch02Offset;
	float m_sysBonePitch02Factor;
	float m_sysBonePitch03Offset;
	float m_sysBonePitch03Factor;
	
	Matrix GetTurretBoneTransform( uint32_t closestTurret, uint32_t boneID ) const;

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

	bool m_useLowLodFiringTransform;
	Vector3 m_lowLodFiringEffectTranslation;
	Vector3 m_lowLodFiringEffectScale;
	Quaternion m_lowLodFiringEffectRotation;

	bool m_chooseRandomLocator;
	bool m_randomizeExplosionRotation;

	// ambient effects
	IEveSpaceObjectChildPtr m_ambientEffect;
	EveChildInstanceContainerPtr m_generatedDistributedAmbientEffect;
	Matrix m_ambientOffsetMatrix; // used while editing
	bool m_ambientEffectEditingMode; // used for editing

	// Audio specific attributes
	bool m_playMovementSound;
	TriObserverLocalPtr m_turretMovementObserver;
	std::wstring m_idleToTargetingMovementAudioEvent;
	std::wstring m_targetingToIdleMovementAudioEvent;
};

TYPEDEF_BLUECLASS( EveTurretSet );
BLUE_DECLARE_VECTOR( EveTurretSet );

#endif // EveTurretSet_H
