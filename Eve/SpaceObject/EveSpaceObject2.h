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
#include "Eve/EveAnimationData.h"

// forwards
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE_VECTOR( EveSpaceObject2 );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );
BLUE_DECLARE( EveSpaceObjectDecal );
BLUE_DECLARE_VECTOR( EveSpaceObjectDecal );
BLUE_DECLARE( EveMeshOverlayEffect );
BLUE_DECLARE_VECTOR( EveMeshOverlayEffect );
BLUE_DECLARE( EveLocator2 );
BLUE_DECLARE_VECTOR( EveLocator2 );
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );
BLUE_DECLARE( EveSpotlightSet );
BLUE_DECLARE_VECTOR( EveSpotlightSet );
BLUE_DECLARE( EvePlaneSet );
BLUE_DECLARE_VECTOR( EvePlaneSet );
BLUE_DECLARE( Tr2GrannyAnimation );

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );

BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE_VECTOR( TriObserverLocal );

BLUE_DECLARE( Tr2GPUParticleEmitter );
BLUE_DECLARE_VECTOR( Tr2GPUParticleEmitter );

BLUE_DECLARE( Tr2BindingVector3 );

BLUE_DECLARE( EveAnimationStateSequencer );

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
//   This class holds the per object data for space objects
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveSpaceObjectPerObjectData : public Tr2PerObjectDataWithPersistentBuffers<EveSpaceObject2>
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	unsigned int m_psPointLightCount;
};



// --------------------------------------------------------------------------------
// Description:
//   EveSpaceObject2 is a base class for objects in space in Eve - namely stations and ships.
//   EveSpaceObject2s can have attachments bound to locators. The locators can either be
//   transforms stored in the 'locators' array, or they can be joints in the skeleton (if
//   it has any). Note that the locator transforms are expected to be static and are not
//   updated per frame, for performance reasons.
// --------------------------------------------------------------------------------
class EveSpaceObject2 :
	public IInitialize,
	public ITr2Renderable,
	public IEveSpaceObject2,
	public IEveShadowCaster,
	public IEveLightReceiver,
	public IBlueAsyncResNotifyTarget,
    public ITr2Pickable,
	public ITriTargetable,
	public IWorldPosition
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
	Tr2MeshPtr GetMesh() const { return m_mesh; }
	Tr2Mesh* GetHighDetailMesh() const;
	Tr2Mesh* GetMediumDetailMesh() const;
	Tr2Mesh* GetLowDetailMesh() const;
	void SetMesh( Tr2MeshPtr mesh );
	void SetHighDetailMesh( Tr2Mesh* mesh );
	void SetMediumDetailMesh( Tr2Mesh* mesh );
	void SetLowDetailMesh( Tr2Mesh* mesh );

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

	// Update the transformed damage locator positions
	void UpdateDamageLocatorPositions();
	void UpdateImpactDirections();

	void UnloadLodIfNeeded( Be::Time time );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );

	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	virtual void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;
	virtual void GetModelCenterWorldPosition( Vector3 &position, Be::Time t );
	virtual void GetCurrentModelCenterWorldPosition( Vector3 &position );
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	virtual void GetLocalToWorldTransform( Matrix &transform );

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

	void SetupResourceLoadCallbacks();

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
	virtual IRoot* GetID() { return this->GetRawRoot(); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriTargetable
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3* out, int index );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetInterestingDamageLocatorIndex( const Vector3 &position ) const;
	int GetGoodDamageLocatorIndex( const Vector3& position );

	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	/////////////////////////////////////////////////////////////////////////////////////
	virtual const Vector3* GetWorldPosition();

	//For stateful GPU particles
	ITriVectorFunctionPtr GetPositionFunction();
	
	Vector3 GetModelWorldPosition();
	Tr2GrannyAnimationPtr GetAnimationController() { return m_animationUpdater; }

	// bounding sphere
	void SetBoundingSphereInformation( const Vector4* centerAndRadius );
	Be::Result<std::string> GetLocalBoundingBoxFromScript( std::pair<Vector3, Vector3>& result );

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

	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );

protected:
	void FreezeHighDetailMesh();

	void PrepareForAnimation();
	void FreeAnimationData();
	void SelectMeshLevelOfDetail();
	void GetBatchesFromMeshAreaVector( const Tr2MeshAreaVector* areas, ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData ) const;
	void GetSortedBatchesFromMeshAreaVector( const Tr2MeshAreaVector* areas, ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData ) const;
	void GetBatchesFromOverlayVector( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, TriBatchType batchType );

	//GPUParticle ship explosion test - not moving into ITriTargetable until tested further.
	unsigned GetDamageLocatorCount() const;
	Vector3 GetDamageLocator( unsigned index ) const;
	Vector3 GetTransformedDamageLocator( unsigned index ) const;

	CcpMutex& GetObjectMutex();
	void UpdateWorldTransform( Be::Time time );
protected:
	friend class EveShip2Builder;

	std::string m_name;

	bool m_update;
	bool m_display;
	bool m_debugShowBoundingBox;
	bool m_debugShowMeshAreaBoundingBox;
	bool m_debugRenderDebugInfoForChildren;
	bool m_allowLodSelection;

	Matrix m_worldTransform;
	Vector3 m_worldPosition; // used to expose the position of the object to python
	float m_modelScale;

	Tr2MeshPtr m_mesh;


	/////////////////////////////////////////////////////////////////////////////////////
	// per-object data
	Tr2PersistentPerObjectData<EveSpaceObject2> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveSpaceObject2> m_perObjectDataPs;
	EveSpaceSceneLightMgrPtr m_lightManager;
	unsigned m_psPointLightCount;
	Vector4 m_spaceObjectData;

	/////////////////////////////////////////////////////////////////////////////////////
	// lod level
	LodLevel m_lodLevel;
	LodLevel m_lodLevelWithChildren;
	IBlueObjectProxyPtr m_highDetailMesh;
	IBlueObjectProxyPtr m_mediumDetailMesh;
	IBlueObjectProxyPtr m_lowDetailMesh;
	// lod level faders
	float m_highDetailLodFader;

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

	// Bounding Sphere Stuff - used by the camera
	Vector3 m_boundingSphereCenter;
	float m_boundingSphereRadius;

	Vector4 m_boundingSphereWorld;

	bool m_allAreasCastShadow;
	void CacheAllAreasCastShadow();

	float GetBoundingSphereRadius();
	Vector3 GetBoundingSphereCenter();
	bool RebuildBoundingSphereInformation();

	Vector3 m_localAabbMin;
	Vector3 m_localAabbMax;

	/////////////////////////////////////////////////////////////////////////////////////
	// overlay effects
	std::vector<TriRenderBatchAreaBlock> m_overlayMeshAreaBlocks;
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

	unsigned m_allocatedDamageLocatorCount;
	unsigned m_persistedImpactDirectionCount;
	XMMATRIX* m_alignedTransformMatrix;
	XMVECTOR* m_damageLocatorPositions;
	bool m_damageLocatorsUpdatedThisFrame;
	XMVECTOR* m_transformedDamageLocators;
	XMVECTOR* m_transformedImpactDirections;
	bool m_impactDirectionsUpdatedThisFrame;

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

	Be::Time m_lastCurveUpdateTime;
	// Children transforms

	Vector3d m_previousPosition;
	Tr2BindingVector3Ptr m_positionDelta;
	PTriCurveSetVector m_curveSets;

private:
	EveAnimationStateSequencerPtr m_animationSequencer;
};

TYPEDEF_BLUECLASS( EveSpaceObject2 );

#endif // EveSpaceObject2_H
