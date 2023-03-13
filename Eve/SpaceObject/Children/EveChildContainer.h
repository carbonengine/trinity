////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveChildContainer_H
#define EveChildContainer_H

#include "IEveSpaceObjectChild.h"
#include "IEveEffectChildrenOwner.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "TransformModifiers/IEveChildTransformModifier.h"
#include "IEveInheritPropertiesOwner.h"
#include "ITr2CurveSetOwner.h"
#include "EveChildInheritProperties.h"
#include "ITr2SoundEmitterOwner.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Eve/SpaceObject/Attachments/Sets/IEveSpaceObjectAttachmentOwner.h"
#include "Eve/SpaceObject/Attachments/Sets/IEveSpaceObjectAttachment.h"


BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE_VECTOR( TriObserverLocal );
BLUE_DECLARE( Tr2Light );
BLUE_DECLARE_VECTOR( Tr2Light );
BLUE_DECLARE_INTERFACE( ITr2Controller );
BLUE_DECLARE_IVECTOR( ITr2Controller );

BLUE_DECLARE_INTERFACE( IEveFxAttribute );
BLUE_DECLARE_IVECTOR( IEveFxAttribute );

BLUE_CLASS( EveChildContainer ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public ITr2CurveSetOwner,
	public IInitialize,
	public INotify,
	public IListNotify,
	public IEveEffectChildrenOwner,
	public ITr2DebugRenderable,
	public IShaderConfigurer,
	public ITr2SoundEmitterOwner,
	public ITr2ControllerOwner,
	public IEveInheritPropertiesOwner,
	public IEveSpaceObjectAttachmentOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildContainer( IRoot* lockobj = NULL );
	~EveChildContainer();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );
	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * val );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	bool IsAlwaysOn() const override;
	void AddTransformModifier( IEveChildTransformModifier* modifier ) override;
    void SetProceduralContainerVariable( const char* name, float value ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveEffectChildrenOwner
	IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const;
	void AddToEffectChildrenList( IEveSpaceObjectChild* child );
	void RemoveFromEffectChildrenList( IEveSpaceObjectChild* child );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name ) override;
	void StartControllers();
	bool GetControllerValueByName( const char* name, float& out );
	void AddController( ITr2Controller * controller );

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2SoundEmitterOwner
	ITr2AudEmitterPtr FindSoundEmitter( const char* name ) override;
	void AddObserver( TriObserverLocalPtr observer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachmentOwner
	void AddAttachment( IEveSpaceObjectAttachment * attachment );
	void ClearAttachments();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	bool HasTransparentBatches();
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );

	const char* GetName() const;
	void SetName( const char* name );

	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, Matrix& parentTransform );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;

	void SetOrigin( Origin origin );

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void SetAlwaysOn( bool alwaysOn );

	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	void UpdateCurveSet( const std::string& name, Be::Time time );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;

	void PlayAllCurveSets() override;
	void StopAllCurveSets() override;

	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	void GetWorldVelocity( Vector3& velocity ) const;
	void SetInheritProperties( const Color* colorSet ) override;

	float GetOwnerMaxSpeed() const;

	void SetAnimationOwner( ITr2GrannyAnimationOwner* animationOwner );

	enum DisplayQualityModifier
	{
		ONLY_REFLECTIONS = 6,
		SHADER_ALL = 5,
		SHADER_HIGHMID = 3,
		SHADER_LOWMID = 1,
		SHADER_HIGH = 4,
		SHADER_MED = 2,
		SHADER_LOW = 0,
	};
	void SetDisplayQualityModifier( DisplayQualityModifier filter );
	void SetIsPlacementRoot( bool isPlacementRoot );

	bool Empty() const;

	PIEveSpaceObjectChildVector m_objects;

protected:
	void DoUpdateAsyncronous( EveUpdateContext & updateContext, const EveChildUpdateParams& params );

	bool IsRendering() const;
	bool IsUpdating() const;

	bool HasRenderables() const;

	BlueSharedString m_name;
	PTriCurveSetVector m_curveSets;
	PTriObserverLocalVector m_observers;
	PTr2LightVector m_lights;
	PITr2ControllerVector m_controllers;
	TrackableStdUnorderedMap<std::string, float> m_controllerVariables;
	PIEveChildTransformModifierVector m_transformModifiers;
	Vector3 m_worldVelocity;
	float m_activationStrength;
	float m_ownerMaxSpeed;
	bool m_display;
	DisplayQualityModifier m_displayFilter;
	bool m_isAlwaysOn;
	bool m_isPlacementRoot;
	EveChildInheritPropertiesPtr m_inheritProperties;
	Origin m_origin;
	PIEveFxAttributeVector m_fxAttributes;

	ITr2GrannyAnimationOwnerPtr m_animationOwner;
	PIEveSpaceObjectAttachmentVector m_attachments;

	// per-object data only used for the attachments
	Tr2PersistentPerObjectData<EveChildContainer> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveChildContainer> m_perObjectDataPs;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;
};

TYPEDEF_BLUECLASS( EveChildContainer );

#endif