#pragma once

#ifndef EveEffectRoot2_h
#define EveEffectRoot2_h

#include "include/ITriFunction.h"
#include "IWorldPosition.h"
#include "EveTransform.h"
#include "EveLODHelper.h"
#include "IEveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/SpaceObject/Children/IEveEffectChildrenOwner.h"
#include "Tr2ShLightingManager.h"
#include "Include/ITriTargetable.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"
#include "Shader/IShaderConfigurer.h"
#include "ITr2SoundEmitterOwner.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Lights/ITr2LightOwner.h"
#include "EveEntity.h"

BLUE_DECLARE( Tr2Light );
BLUE_DECLARE_VECTOR( Tr2Light );

BLUE_DECLARE_INTERFACE( ITr2Controller );
BLUE_DECLARE_IVECTOR( ITr2Controller );

BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );

BLUE_DECLARE( EveEffectRoot2 );

BLUE_CLASS( EveEffectRoot2 ):
	public IWorldPosition,
	public IEveSpaceObject2,
	public IInitialize,
	public ITr2SecondaryLightSource,
	public ITriTargetable,
	public ITr2DebugRenderable,
	public ITr2CurveSetOwner,
	public INotify,
	public IListNotify,
	public IEveEffectChildrenOwner,
	public IShaderConfigurer,
	public ITr2SoundEmitterOwner,
	public ITr2ControllerOwner,
	public ITr2LightOwner,
	public EveEntity

{
public:
    EXPOSE_TO_BLUE();
	using IEveSpaceObject2::Lock;
	using IEveSpaceObject2::Unlock;

	EveEffectRoot2( IRoot* lockobj = NULL );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void UpdateVisibility(  const TriFrustum& frustum, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const;
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t );
	void GetModelCenterWorldPosition( Vector3 &position ) const;
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	void GetLocalToWorldTransform( Matrix &transform ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );
    void SetProceduralContainerVariable( const char *name, float value ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager& lightManager ) const;
	void AddLight( Tr2Light* light );
	void ClearLights( );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2SecondaryLightSource
	virtual void RegisterSecondaryLightSource( Tr2ShLightingManager& );
	virtual void UnregisterSecondaryLightSource( Tr2ShLightingManager& );

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents( );
	void UnRegisterComponents( );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITriTargetable
	unsigned int GetDamageLocatorCount() const;
	int GetClosestDamageLocatorIndex( const Vector3* position );
	bool GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace );
	bool GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace );
	void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out );
	int GetGoodDamageLocatorIndex( const Vector3& position );
	float GetRadius() const;
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size );
	bool UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex );
	bool GetImpactPosition( Vector3& out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon );
	bool HasImpactConfigurationShield() const;


	/////////////////////////////////////////////////////////////////////////////////////
	// IWorldPosition
	virtual Vector3 GetWorldPosition();
	virtual Quaternion GetWorldRotation();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2CurveSetOwner
	virtual void PlayCurveSet( const std::string& name, const std::string& rangeName );
	virtual void StopCurveSet( const std::string& name );
	virtual void UpdateCurveSet( const std::string& name, Be::Time time );
	virtual float GetCurveSetDuration( const std::string& name ) const;
	virtual float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveEffectChildrenOwner
	IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const;
	void AddToEffectChildrenList( IEveSpaceObjectChild* child );
	void RemoveFromEffectChildrenList( IEveSpaceObjectChild* child );
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name ) override;
	void StartControllers();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2SoundEmitterOwner
	ITr2AudEmitterPtr FindSoundEmitter( const char* name ) override;
	void AddObserver( TriObserverLocalPtr observer ) override;

	void Start();
	void Stop();

	PIEveSpaceObjectChildVector& GetChildren();

	void SetTransform( const Matrix& transform );

	void FreezeHighDetailMesh();
	void UpdateControllers();

	bool ShouldReflect() const;


private:
	void UpdateWorldTransform( Be::Time time );
	PTr2ExternalParameterVector m_externalParameters;
	Be::Time m_startTime;
	ITriQuaternionFunctionPtr m_modelRotation;
	ITriVectorFunctionPtr m_modelTranslation;
	Vector4 m_boundingSphere;
	Matrix m_lastUpdateMatrix;
	Matrix m_localTransform;
	float m_secondaryLightingSphereRadiusLocal;
	float m_secondaryLightingSphereRadiusWorld;

	// Lods
	bool m_dynamicLODSelection;
	bool m_changeLOD;
	
	PTr2LightVector m_lights;
	PITr2ControllerVector m_controllers;
	TrackableStdUnorderedMap<std::string, float> m_controllerVariables;
	float GetBoundingSphereRadius() { return m_boundingSphere.w; }

	float m_estimatedSize;
	float m_effectDuration;
	
	IBlueEventListenerPtr m_loadedEventListener;

protected:
	// general
	std::string m_name;
	bool m_display;
	PIEveSpaceObjectChildVector m_effectChildren;

	Vector3 m_scaling;
	Quaternion m_rotation;
	Vector3 m_translation;
	
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;
	
	PTriCurveSetVector m_curveSets;

	// last known results from updating m_ballPosition and m_ballRotation
	Matrix m_worldTransform;

	Color m_secondaryLightingEmissiveColor;

	// PlacementObservers
	PTriObserverLocalVector m_observers;

	// Current LOD level
	Tr2Lod m_lodLevel;

};

TYPEDEF_BLUECLASS( EveEffectRoot2 );

#endif // EveEffectRoot2_h