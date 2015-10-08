#pragma once
#ifndef EveSpaceObject2_H
#define EveSpaceObject2_H



#include "include/ITriTargetable.h"
#include "TriRenderBatch.h"

#include "Eve/IEveShadowCaster.h"
#include "Eve/IEveLightReceiver.h"
#include "Eve/EveSpaceSceneLightMgr.h"
#include "Tr2PerObjectData.h"
#include "ITr2Renderable.h"
#include "Eve/IEveSpaceObject2.h"
#include "IWorldPosition.h"
#include "include/ITriFunction.h"
#include "Eve/IEveTransform.h"
#include "Vector3d.h"
#include "Tr2PersistentPerObjectData.h"
#include "Eve/Animation/EveAnimationData.h"
#include "Tr2ShLightingManager.h"
#include "Eve/SpaceObject/Attachments/EveMeshOverlayEffect.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

// consts
#define EVE_SPACEOBJECT_DIRT_LEVEL_DEFAULT (0.f)

// forwards
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE_VECTOR( EveSpaceObject2 );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( Tr2MeshLod );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );
BLUE_DECLARE( EveLocator2 );
BLUE_DECLARE_VECTOR( EveLocator2 );
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );
BLUE_DECLARE( EveSpotlightSet );
BLUE_DECLARE_VECTOR( EveSpotlightSet );
BLUE_DECLARE( EvePlaneSet );
BLUE_DECLARE_VECTOR( EvePlaneSet );
BLUE_DECLARE( Tr2GrannyAnimation );
BLUE_DECLARE( EveTransform );
BLUE_DECLARE( EveCustomMask );
BLUE_DECLARE( EveImpactOverlay );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );

BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE_VECTOR( TriObserverLocal );

BLUE_DECLARE( Tr2GPUParticleEmitter );
BLUE_DECLARE_VECTOR( Tr2GPUParticleEmitter );

BLUE_DECLARE( Tr2BindingVector3 );

BLUE_DECLARE( EveAnimationSequencer );

BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );

struct granny_skeleton;

class TriFrustum;
class EveSpaceObjectDecalCache;


struct EveDamageLocator
{
	Vector3 m_position;
	Quaternion m_impactDirection;
};

BLUE_DECLARE_STRUCTURE_LIST( EveDamageLocator );

// --------------------------------------------------------------------------------
// Description:
//   This struct holds the per vs data for spaceobjects, excluding bones.
// --------------------------------------------------------------------------------
struct EveSpaceObjectVSData
{
	Matrix worldTransform;
	Matrix worldTransformLast;
	Vector4 miscData;
	Vector4 clipData;
	Vector4 ellpsoidRadii;
	Vector4 ellpsoidCenter;
};

// --------------------------------------------------------------------------------
// Description:
//   This struct holds the per s data for spaceobjects.
// --------------------------------------------------------------------------------
struct EveSpaceObjectPSData
{
	Vector4 miscData;
	Vector4 clipData;
	Vector4 clipDataEx;
	Vector4 shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT];
};

// --------------------------------------------------------------------------------
// Description:
//   EveSpaceObject2 is a base class for objects in space in Eve - namely stations and ships.
//   EveSpaceObject2s can have attachments bound to locators. The locators can either be
//   transforms stored in the 'locators' array, or they can be joints in the skeleton (if
//   it has any). Note that the locator transforms are expected to be static and are not
//   updated per frame, for performance reasons.
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpaceObject2 ):
	public IInitialize,
	public ITr2Renderable,
	public IEveSpaceObject2,
	public IEveShadowCaster,
	public IEveLightReceiver,
	public IBlueAsyncResNotifyTarget,
    public ITr2Pickable,
	public ITriTargetable,
	public IWorldPosition,
	public ITr2ShLightingReceiver,
	public INotify,
	public ITr2SecondaryLightSource
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveSpaceObject2( IRoot* lockobj = NULL );
	~EveSpaceObject2();

	// locator types (static, bone, etc.)
	enum LocatorType
	{
		ELT_TRANSFORM,
		ELT_JOINT,

		ELT_COUNT,
	};

	// Mesh accessors, used by the builder
	Tr2MeshBase* GetMesh() const { return m_mesh; }
	void SetMesh( Tr2MeshBase* mesh );

	void SetMeshLod( Tr2MeshLod* meshLod );

	void PlayAnimation( const char* animName, bool replace, int loopCount, float start, float speed );
	void PlayAnimationOnce( const char* animName );
	void PlayAnimationEx( const char* animName, int loopCount, float start, float speed );
	void ChainAnimation( const char* animName );
	void ChainAnimationEx( const char* animName, int loopCount, float start, float speed );
	void EndAnimation();
	void ClearAnimations();

	// query for the number of bones in the gr2
	int GetBoneCount() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );
	virtual void PrepareShaderData( EveUpdateContext& updateContext );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	virtual void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;
	virtual void GetModelCenterWorldPosition( Vector3 &position, Be::Time t );
	virtual void GetCurrentModelCenterWorldPosition( Vector3 &position );
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	virtual void GetLocalToWorldTransform( Matrix &transform ) const;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void GetLights( Tr2LightManager& lightManager ) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	virtual bool GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables );
	virtual bool IsShadowReceiveEnabled();

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveLightReceiver
	virtual void SetLights( EveSpaceSceneLightMgrPtr lightMgr );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes* p );
	virtual void RebuildCachedData( BlueAsyncRes* p );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t );
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriTargetable
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3* out, int index );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetInterestingDamageLocatorIndex( const Vector3 &position ) const;
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	virtual const Vector3* GetWorldPosition();

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// animation controller
	virtual bool ExecuteAnimationStateCommand( EveAnimationCmd cmd, const std::string& data, const std::map<std::string, float>& parameters );

	/////////////////////////////////////////////////////////////////////////////////////
	// decal
	virtual void FillDecalParentData( EveSpaceObjectDecal::ParentData* pd ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ShLightingReceiver
	virtual void UpdateShLighting( Tr2ShLightingManager& );
	virtual void ClearShLighting();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2SecondaryLightSource
	void RegisterSecondaryLightSource( Tr2ShLightingManager& manager );
	void UnregisterSecondaryLightSource( Tr2ShLightingManager& manager );

	// For stateful GPU particles
	ITriVectorFunctionPtr GetPositionFunction();

	Vector3 GetModelWorldPosition() const;
	Tr2GrannyAnimationPtr GetAnimationController() { return m_animationUpdater; }

	// bounding sphere
	void SetBoundingSphereInformation( const Vector4* centerAndRadius );
	Be::Result<std::string> GetLocalBoundingBoxFromScript( std::pair<Vector3, Vector3>& result );
	void GetShapeEllipsoid( Vector3& center, Vector3& radius );
	void SetShapeEllipsoid( const Vector3* center, const Vector3* radius );

	// access curve sets
	void UpdateCurveSet( const std::string& name, Be::Time time );
	void PlayCurveSet( const std::string& name );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;

	// access spritesets & co
	void AddSpriteSet( EveSpriteSetPtr newSpriteSet );
	void AddSpotlightSet( EveSpotlightSetPtr newSpotlightSet );
	void AddPlaneSet( EvePlaneSetPtr newPlaneSet );
	void AddDecal( EveSpaceObjectDecalPtr newDecal );
	void AddCurveSet( TriCurveSetPtr newCurveSet );
	void AddLocator( EveLocator2* newLocator );
	void SetDamageLocators( const EveDamageLocator* damageLocators, size_t damageLocatorCount );

	// access to shadows
	void SetShadowEffect( Tr2EffectPtr newShadowEffect );

	// access to children
	void AddToChildrenList( EveTransformPtr transform );

	// add to children
	void AddToEffectChildrenList( IEveSpaceObjectChildPtr child );

	// access to curves
	void SetModelRotationCurve( ITriQuaternionFunctionPtr rotationCurve );
	void SetModelTranslationCurve( ITriVectorFunctionPtr translationCurve );
	ITriQuaternionFunctionPtr GetModelRotationCurve() const { return m_modelRotation; };
	ITriVectorFunctionPtr GetModelTranslationCurve() const { return m_modelTranslation; };

	// access to dna
	void SetDnaString( const char* dna );

	// access to impacts
	int CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime );
	bool UpdateShieldImpact( const Vector3& direction, int shieldImpactIndex );
	bool GetShieldImpactPosition( Vector3& out, int shieldImpactIndex ) const;
	int CreateArmorImpact( int damageLocatorIndex );

	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );

	void GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const;
protected:
	// LODing
	void UnloadLodIfNeeded( Be::Time time );
	void FreezeHighDetailMesh();

	// damage locators
	unsigned GetDamageLocatorCount() const;
	Vector3 GetDamageLocator( unsigned index ) const;
	Vector3 GetTransformedDamageLocator( unsigned index ) const;
	void UpdateDamageLocatorPositions();
	void UpdateDamageLocatorDirections();

	void PrepareForAnimation();
	void FreeAnimationData();
	void SelectMeshLevelOfDetail();
	void GetSortedBatchesFromMeshAreaVector( const Tr2MeshAreaVector* areas, ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData ) const;
	void GetBatchesFromOverlayVector( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, TriBatchType batchType );

	CcpMutex& GetObjectMutex();
	void UpdateWorldTransform( Be::Time time );
protected:
	friend class EveShip2Builder;

	std::string m_name;
	std::string m_dna;

	bool m_update;
	bool m_display;
	bool m_debugShowBoundingBox;
	bool m_debugShowMeshAreaBoundingBox;
	bool m_debugRenderDebugInfoForChildren;
	bool m_debugShowDynamicBounds;
	bool m_allowLodSelection;

	Matrix m_worldTransform;
	Vector3 m_worldPosition; // used to expose the position of the object to python
	float m_modelScale;

	Tr2MeshBasePtr m_mesh;
	Tr2MeshLodPtr m_meshLod;

	/////////////////////////////////////////////////////////////////////////////////////
	// per-object data
	Tr2PersistentPerObjectData<EveSpaceObject2> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveSpaceObject2> m_perObjectDataPs;
	EveSpaceSceneLightMgrPtr m_lightManager;
	unsigned m_psPointLightCount;
	Vector4 m_spaceObjectMiscData;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;

	/////////////////////////////////////////////////////////////////////////////////////
	// lod level
	Tr2Lod m_lodLevel;
	Tr2Lod m_lodLevelWithChildren;

	TriGeometryResPtr m_geometryResFromMesh;

	// Set to true if the object is inside the frustum
	bool m_isInFrustum;
	bool m_isInFrustumInShadow;

	// Set to true if the object is inside the frustum and its estimated
	// pixel diameter is above the visibility threshold
	bool m_isVisible;
	bool m_isMeshVisible;

	// Estimated pixel diameter, based on bounding sphere of the geometry. Only
	// valid if we're inside the frustum.
	float m_estimatedPixelDiameter;
	float m_estimatedPixelDiameterInShadow;
	float m_estimatedPixelDiameterWithChildren;
	
	Tr2GrannyAnimationPtr m_animationUpdater;

	/////////////////////////////////////////////////////////////////////////////////////
	// bounding sphere info
	Vector3 m_boundingSphereCenter;
	float m_boundingSphereRadius;

	Vector4 m_boundingSphereWorld;
	Vector4 m_dynamicBoundingSphere;

	bool m_allAreasCastShadow;
	void CacheAllAreasCastShadow();

	float GetBoundingSphereRadius() const;
	Vector3 GetBoundingSphereCenter() const;

	Vector4 CalculateSkinnedBoundingSphere();
	std::pair<Vector3, Vector3> CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform );

	bool RebuildBoundingSphereInformation();

	/////////////////////////////////////////////////////////////////////////////////////
	// bounding box info
	Vector3 m_localAabbMin;
	Vector3 m_localAabbMax;

	/////////////////////////////////////////////////////////////////////////////////////
	// shape ellipsoid info
	Vector3 m_shapeEllipsoidCenter;
	Vector3 m_shapeEllipsoidRadius;

	/////////////////////////////////////////////////////////////////////////////////////
	// overlay effects
	std::vector<TriRenderBatchAreaBlock> m_overlayMeshAreaBlocks[EveMeshOverlayEffect::TYPE_COUNT];
	PEveMeshOverlayEffectVector m_overlayEffects;

	/////////////////////////////////////////////////////////////////////////////////////
	// decals
	PEveSpaceObjectDecalVector m_decals;
	EveSpaceObjectDecalCache* m_decalCache;
	bool DisplayDecals() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Locators
	PEveLocator2Vector m_locators;
	const Matrix* GetLocatorTransform( LocatorType lt, unsigned int lix );
	LocatorType DetermineLocatorType( const char* locatorName, unsigned int& locatorIndex ) const;
	bool FindLocatorTransformByName( const char* name, unsigned int& ix ) const;
	bool FindLocatorJointByName( const char* name, unsigned int& ix ) const;
	unsigned int CountLocatorsByPrefix( const char* namePrefix ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Damage
	EveImpactOverlayPtr m_impactOverlay;

	/////////////////////////////////////////////////////////////////////////////////////
	// PlacementObservers
	PTriObserverLocalVector m_observers;

	// It would be really, really nice not to have to hold on to the ball twice
	// but we can't do it without writing another interface, or linking to destiny
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	ITriQuaternionFunctionPtr m_modelRotation;
	ITriVectorFunctionPtr m_modelTranslation;

	// Damage locators are positions, relative to the origin of the object. They are persisted
	// as a list of Vector3 and copied to an array of XMVECTORs on load time for more
	// efficient per-frame calculations of transformed damage locator positions.

	PEveDamageLocatorStructureList m_persistedDamageLocators;

	/////////////////////////////////////////////////////////////////////////////////////
	// Dynamic lighting
	PTr2PointLightVector m_lights;

	unsigned m_allocatedDamageLocatorCount;
	unsigned m_persistedImpactDirectionCount;
	XMMATRIX* m_alignedTransformMatrix;
	XMVECTOR* m_damageLocatorPositions;
	bool m_damageLocatorsUpdatedThisFrame;
	XMVECTOR* m_transformedDamageLocators;
	XMVECTOR* m_transformedImpactDirections;
	bool m_impactDirectionsUpdatedThisFrame;

	Color m_albedoColor;
	float m_secondaryLightingSphereRadius;

	/////////////////////////////////////////////////////////////////////////////////////
	// Shadow related things
	Tr2EffectPtr m_shadowEffect;
	bool m_enableShadow;

	/////////////////////////////////////////////////////////////////////////////////////
	// Sprites and spotlights and planes
	PEveSpriteSetVector m_spriteSets;
	PEveSpotlightSetVector m_spotlightSets;
	PEvePlaneSetVector m_planeSets;

	//// Helper function for determining visibility. Tests the bounding sphere of
	//// the first mesh in m_geometryResource against the given frustum.
	//bool IsInFrustum( const TriFrustum& frustum );

	void RebuildDamageLocatorCache();
	//void DisplayShadowMap();

	/////////////////////////////////////////////////////////////////////////////////////
	// Particle Systems
	PTr2GPUParticleEmitterVector m_particleEmittersGPU;

	/////////////////////////////////////////////////////////////////////////////////////
	// children
	PIEveTransformVector m_children;
	bool m_displayChildren;
	virtual bool DisplayChildren() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Effect children
	PIEveSpaceObjectChildVector m_effectChildren;

	/////////////////////////////////////////////////////////////////////////////////////
	// custom masks
	EveCustomMaskPtr m_customMask;

	/////////////////////////////////////////////////////////////////////////////////////
	// dirt levels
	float m_dirtLevel;



	Be::Time m_lastCurveUpdateTime;
	// Children transforms

	Vector3d m_previousPosition;
	Tr2BindingVector3Ptr m_positionDelta;
	PTriCurveSetVector m_curveSets;
	
private:
	bool m_isAnimated;
	EveAnimationSequencerPtr m_animationSequencer;
};

TYPEDEF_BLUECLASS( EveSpaceObject2 );

#endif // EveSpaceObject2_H
