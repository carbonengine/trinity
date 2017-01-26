#pragma once
#ifndef EveSpaceObject2_H
#define EveSpaceObject2_H



#include "include/ITriTargetable.h"
#include "TriRenderBatch.h"

#include "Eve/IEveShadowCaster.h"
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
#include "Eve/SpaceObject/Attachments/EveImpactOverlay.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/SpaceObject/Utils/EveLocatorSets.h"
#include "Tr2ImpostorManager.h"
#include "Tr2DebugRenderer.h"

// consts
#define EVE_SPACEOBJECT_DIRT_LEVEL_DEFAULT (0.f)
#define EVE_SPACEOBJECT_CUSTOWMASK_MAX (2)

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
BLUE_DECLARE( EveSpriteLineSet );
BLUE_DECLARE_VECTOR( EveSpriteLineSet );
BLUE_DECLARE( Tr2GrannyAnimation );
BLUE_DECLARE( EveTransform );
BLUE_DECLARE( EveCustomMask );
BLUE_DECLARE_VECTOR( EveCustomMask );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( EveLocatorSets );
BLUE_DECLARE_VECTOR( EveLocatorSets );
BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE_VECTOR( TriObserverLocal );

BLUE_DECLARE( Tr2GPUParticleEmitter );
BLUE_DECLARE_VECTOR( Tr2GPUParticleEmitter );

BLUE_DECLARE( Tr2BindingVector3 );

BLUE_DECLARE( EveAnimationSequencer );

BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );

BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );

struct granny_skeleton;

class TriFrustum;
class EveSpaceObjectDecalCache;
struct Locator;


struct EveDamageLocator
{
	Vector3 m_position;
	Quaternion m_impactDirection;
	int m_boneIndex;
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
	Vector4 shipData;
	Vector4 clipData;
	Vector4 ellpsoidRadii;
	Vector4 ellpsoidCenter;
	Matrix customMaskMatrix[ EVE_SPACEOBJECT_CUSTOWMASK_MAX ];
	Vector4 customMaskData[ EVE_SPACEOBJECT_CUSTOWMASK_MAX ];
};

// --------------------------------------------------------------------------------
// Description:
//   This struct holds the per s data for spaceobjects.
// --------------------------------------------------------------------------------
struct EveSpaceObjectPSData
{
	Vector4 shipData;
	Vector4 clipData;
	Vector4 miscData;
	Vector4 shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT];
	Vector4 customMaskMaterialIDs[ EVE_SPACEOBJECT_CUSTOWMASK_MAX ];
	Vector4 customMaskTargets[ EVE_SPACEOBJECT_CUSTOWMASK_MAX ];
};

// ---------------------------------------------------------------------------------------
//  Description:
//    Given a pointer to a mesh area vector, gathers TriGeometryBatches for each of the
//    areas. Order of batches is sorted, based upon distance to camera.
// Arguments:
//   areas - mesharea vector to collect from
//   batches - accumulator for the new batches
//   perObjectData - the per-object data for these batches
//   mesh - mesh containing the geometry to be rendered
//   worldTransform - world transform of the object the areas/mesh belong to
// ---------------------------------------------------------------------------------------
void GetSortedBatchesFromMeshAreaVector( const Tr2MeshAreaVector* areas, 
										 ITriRenderBatchAccumulator* batches, 
										 const Tr2PerObjectData* perObjectData,
										 const Tr2MeshBase* mesh,
										 const Matrix* worldTransform );

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
	public IBlueAsyncResNotifyTarget,
    public ITr2Pickable,
	public ITriTargetable,
	public IWorldPosition,
	public ITr2ShLightingReceiver,
	public INotify,
	public ITr2SecondaryLightSource,
	public ITr2ImpostorSource,
	public ITr2DebugRenderable
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

	// Actually submit renderables to the list, called from GetRenderables
	virtual void PushRenderables( std::vector<ITr2Renderable*>& renderables );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );
	virtual void UpdateVisibility(  const TriFrustum& frustum, const Matrix& parentTransform );
	virtual void PrepareShaderData( EveUpdateContext& updateContext );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	virtual void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t );
	virtual void GetModelCenterWorldPosition( Vector3 &position ) const;
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	virtual void GetLocalToWorldTransform( Matrix &transform ) const;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );
	virtual void GetLights( Tr2LightManager& lightManager ) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	virtual bool GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables );
	virtual bool IsShadowReceiveEnabled();

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
	unsigned int GetDamageLocatorCount() const;
	int GetClosestDamageLocatorIndex( const Vector3* position );
	virtual bool GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace );
	virtual bool GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	int CreateImpactFromPosition( const Vector3& position, const Vector3& direction, float lifeTime, float size );

	bool UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex );
	void GetImpactPosition( Vector3& out, int damageLocatorIndex, const Vector3& direction );
	bool HasImpactConfigurationShield() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	virtual const Vector3* GetWorldPosition();

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// animation controller
	virtual bool ExecuteAnimationStateCommand( const EveAnimationCommand& cmd, const std::map<std::string, float>& parameters );

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

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ImpostorSource
	virtual void GetImpostorBatches( const TriFrustum& frustum, std::map<TriBatchType, ITriRenderBatchAccumulator*>& batches );
	virtual float GetRenderPriority( const ImpostorHash& oldHash, const ImpostorHash& newHash ) const;
	virtual bool GetImpostorBoundingSphere( Vector4& sphere ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
    virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
    virtual void RenderDebugInfo( Tr2DebugRenderer& renderer );

	// For stateful GPU particles
	ITriVectorFunctionPtr GetPositionFunction();

	Vector3 GetModelWorldPosition() const;
	void GetWorldVelocity( Vector3& velocity ) const;
	Tr2GrannyAnimationPtr GetAnimationController() { return m_animationUpdater; }
	bool IsAnimated() const; 

	// bounding sphere
	void SetBoundingSphereInformation( const Vector4* centerAndRadius );
	Be::Result<std::string> GetLocalBoundingBoxFromScript( std::pair<Vector3, Vector3>& result );
	void GetShapeEllipsoid( Vector3& center, Vector3& radius );
	void SetShapeEllipsoid( const Vector3* center, const Vector3* radius );

	// access to visiblity
	float GetEstimatedPixelDiameter() const;
	bool IsInFrustum() const;
	bool IsImpostor() const;

	// access curve sets
	void UpdateCurveSet( const std::string& name, Be::Time time );
	void PlayCurveSet( const std::string& name );
	void StopCurveSet( const std::string& name );
	float GetCurveSetDuration( const std::string& name ) const;

	// access spritesets & co
	void AddSpriteSet( EveSpriteSetPtr newSpriteSet );
	void AddSpotlightSet( EveSpotlightSetPtr newSpotlightSet );
	void AddPlaneSet( EvePlaneSetPtr newPlaneSet );
	void AddSpriteLineSet( EveSpriteLineSetPtr newSpriteLineSet );
	void AddDecal( EveSpaceObjectDecalPtr newDecal );
	void AddCurveSet( TriCurveSetPtr newCurveSet );
	void AddLocator( EveLocator2* newLocator );
	void AddOverlayEffect( EveMeshOverlayEffect* newOverlayEffect );
	void RemoveOverlayEffect( EveMeshOverlayEffect* newOverlayEffect );
	void AddLocatorSet( const char* name, const Locator* locators, size_t locatorCount );
	void SetDamageLocators( const EveDamageLocator* damageLocators, size_t damageLocatorCount );
	Vector3 GetDamageLocator( uint32_t index ) const;
	Vector3 GetDamageLocatorDirection( uint32_t index ) const;
	Vector3 GetDamageLocatorDirectionLocal( uint32_t index ) const { return GetDamageLocatorDirection( index ); }
	const LocatorStructureList* GetLocatorsForSet( const char* setName ) const;
	void AddCustomMask( EveCustomMaskPtr newCustomMask );

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
	void SetImpactOverlay( EveImpactOverlayPtr overlay );
	void SetImpactDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts );
	void SetImpactAnimation( const std::string& name, bool enable, float duration );
	void ClearImpactDamage();

	// dynamic bounding sphere
	void EnableDynamicBoundingSphere( bool enable );

	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );

	void GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const;

	// external parameters
	void AddExternalParameter( Tr2ExternalParameter* externalParameter );

protected:
	// LODing
	void UnloadLodIfNeeded( Be::Time time );
	void FreezeHighDetailMesh();

	// damage locators
	Vector3 GetTransformedDamageLocator( uint32_t index );
	Vector3 GetTransformedDamageLocatorDirection( uint32_t index );
	bool IsDamageLocatorFacingPosition( uint32_t index, const Vector3& posInObjectSpace );

	void PrepareForAnimation();
	void FreeAnimationData();
	void SelectMeshLevelOfDetail();
	void GetBatchesFromOverlayVector( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, TriBatchType batchType );

	virtual void UpdateWorldTransform( Be::Time time );
	friend class EveShip2Builder;

	std::string m_name;
	std::string m_dna;

	bool m_update;
	bool m_display;
	bool m_allowLodSelection;

	Matrix m_worldTransform;
	Matrix m_invWorldTransform;
	Vector3 m_worldPosition; // used to expose the position of the object to python
	Vector3 m_worldVelocity;
	Quaternion m_worldRotation;
	float m_modelScale;

	Tr2MeshBasePtr m_mesh;
	Tr2MeshLodPtr m_meshLod;

	/////////////////////////////////////////////////////////////////////////////////////
	// per-object data
	Tr2PersistentPerObjectData<EveSpaceObject2> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveSpaceObject2> m_perObjectDataPs;
	Vector4 m_spaceObjectShipData;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;

	/////////////////////////////////////////////////////////////////////////////////////
	// lod level
	Tr2Lod m_lodLevel;
	Tr2Lod m_lodLevelWithChildren;

	TriGeometryResPtr m_geometryResFromMesh;

	bool m_impostorMode;
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

	Vector3 m_boundingSphereWorldCenter;
	float m_boundingSphereWorldRadius;
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
	int m_lastDamageLocatorHit;

	/////////////////////////////////////////////////////////////////////////////////////
	// PlacementObservers
	PTriObserverLocalVector m_observers;

	// It would be really, really nice not to have to hold on to the ball twice
	// but we can't do it without writing another interface, or linking to destiny
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	ITriQuaternionFunctionPtr m_modelRotation;
	ITriVectorFunctionPtr m_modelTranslation;

	/////////////////////////////////////////////////////////////////////////////////////
	// locator sets
	PEveLocatorSetsVector m_locatorSets;
	PEveDamageLocatorStructureList m_persistedDamageLocators;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// Dynamic lighting
	PTr2PointLightVector m_lights;

	
	Color m_albedoColor;
	float m_secondaryLightingSphereRadius;

	/////////////////////////////////////////////////////////////////////////////////////
	// Shadow related things
	Tr2EffectPtr m_shadowEffect;
	bool m_enableShadow;

	/////////////////////////////////////////////////////////////////////////////////////
	// Sprites and spotlights and planes and spritelines
	PEveSpriteSetVector m_spriteSets;
	PEveSpotlightSetVector m_spotlightSets;
	PEvePlaneSetVector m_planeSets;
	PEveSpriteLineSetVector m_spriteLineSets;

	/////////////////////////////////////////////////////////////////////////////////////
	// children
	PIEveTransformVector m_children;	
	virtual bool DisplayChildren() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Effect children
	PIEveSpaceObjectChildVector m_effectChildren;

	/////////////////////////////////////////////////////////////////////////////////////
	// custom masks
	PEveCustomMaskVector m_customMasks;

	/////////////////////////////////////////////////////////////////////////////////////
	// dirt levels
	float m_dirtLevel;
	
	Be::Time m_lastCurveUpdateTime;
	Be::Time m_lastUpdateTransformTime;
	// Children transforms

	Vector3d m_previousPosition;
	Tr2BindingVector3Ptr m_positionDelta;
	PTriCurveSetVector m_curveSets;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// Object space damage locator information
	virtual Vector3 GetObjectSpaceDamageLocatorPosition( uint32_t index ) const;
	virtual Vector3 GetObjectSpaceDamageLocatorDirection( uint32_t index ) const;

	
	/////////////////////////////////////////////////////////////////////////////////////
	// Observer position
	virtual Matrix GetObserverTransform();
private:

#if BLUE_WITH_PYTHON
	static PyObject* PyTransformLocators( PyObject* self, PyObject* args );
#endif

	bool m_dynamicBoundingSphereEnabled;
	EveAnimationSequencerPtr m_animationSequencer;

	PTr2ExternalParameterVector m_externalParameters;

};

TYPEDEF_BLUECLASS( EveSpaceObject2 );

#endif // EveSpaceObject2_H
