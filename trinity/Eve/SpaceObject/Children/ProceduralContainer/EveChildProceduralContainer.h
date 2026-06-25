// Copyright © 2021 CCP ehf.

#pragma once

#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/SpaceObject/Children/IEveEffectChildrenOwner.h"
#include "Eve/SpaceObject/Children/EveChildTransform.h"
#include "Eve/SpaceObject/Children/TransformModifiers/IEveChildTransformModifier.h"
#include "Eve/SpaceObject/Children/EveChildInheritProperties.h"
#include "SelectionMethods/IEveProceduralSelectionMethod.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"


BLUE_CLASS( EveChildProceduralContainer ) :
	public IEveSpaceObjectChild,
	public ITr2CurveSetOwner,
	public IEveInheritPropertiesOwner,
	public EveChildTransform,
	public ITr2SoundEmitterOwner,
	public IInitialize,
	public INotify,
	public IListNotify,
	public ITr2DebugRenderable,
	public IShaderConfigurer,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildProceduralContainer( IRoot* lockobj = NULL );
	~EveChildProceduralContainer();

	const char* GetName() const override;
	void SetName( const char* name ) override;

	void ConfigureSelectedObject();
	const char* GetMethodVariableName();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChildOwner
	void AddTransformModifier( IEveChildTransformModifier * modifier ) override;
	void SetProceduralContainerVariable( const char* name, float value );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value ) override;
	void HandleControllerEvent( const char* name ) override;
	void StartControllers() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2CurveSetOwner
	void PlayCurveSet( const std::string& name, const std::string& rangeName ) override;
	void StopCurveSet( const std::string& name ) override;
	void UpdateCurveSet( const std::string& name, Be::Time time ) override;
	float GetCurveSetDuration( const std::string& name ) const override;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const override;
	void PlayAllCurveSets() override;
	void StopAllCurveSets() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	void ChangeLOD( Tr2Lod lod ) override;
	ITr2AudEmitterPtr FindSoundEmitter( const char* name ) override;
	void SetInheritProperties( const Color* colorSet ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	//  EveChildTransform
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	//  ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

protected:
	IEveSpaceObjectChildPtr m_selectedObject;
	IEveProceduralSelectionMethodPtr m_selectionMethod;

	BlueSharedString m_name;
	PIEveChildTransformModifierVector m_transformModifiers;
	TrackableStdUnorderedMap<std::string, float> m_proceduralContainerVariables;
	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildProceduralContainer );