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
#include "ITr2CurveSetOwner.h"
#include "EveChildInheritProperties.h"
#include "ITr2SoundEmitterOwner.h"


BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( TriObserverLocal );
BLUE_DECLARE_VECTOR( TriObserverLocal );
BLUE_DECLARE( Tr2PointLight );
BLUE_DECLARE_VECTOR( Tr2PointLight );
BLUE_DECLARE_INTERFACE( ITr2Controller );
BLUE_DECLARE_IVECTOR( ITr2Controller );


BLUE_CLASS( EveChildContainer ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2CurveSetOwner,
	public IInitialize,
	public IListNotify,
	public IEveEffectChildrenOwner,
	public ITr2DebugRenderable,
	public IShaderConfigurer,
	public ITr2SoundEmitterOwner
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
	// IEveSpaceObjectChild
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveEffectChildrenOwner
	IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const;
	void AddToEffectChildrenList( IEveSpaceObjectChild* child );
	void RemoveFromEffectChildrenList( IEveSpaceObjectChild* child );

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

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	void UpdateCurveSet( const std::string& name, Be::Time time );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;

	void PlayAllCurveSets();
	void StopAllCurveSets();
	
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( Tr2DebugRenderer& renderer );

	void SetControllerVariable( const char* name, float value );
	void StartControllers();
	void GetWorldVelocity( Vector3& velocity ) const;
	void SetInheritProperties( const Color* colorSet );

	ITr2SoundEmitter* FindSoundEmitter( const char* name ) override;

	PIEveSpaceObjectChildVector m_objects;
protected:
	BlueSharedString m_name;
	PTriCurveSetVector m_curveSets;
	PTriObserverLocalVector m_observers;
	PTr2PointLightVector m_lights;
	PITr2ControllerVector m_controllers;
	TrackableStdUnorderedMap<std::string, float> m_controllerVariables;

	PIEveChildTransformModifierVector m_transformModifiers;
	Vector3 m_worldVelocity;
	bool m_display;
	bool m_hideOnLowQuality;
	EveChildInheritPropertiesPtr m_inheritProperties;
};

TYPEDEF_BLUECLASS( EveChildContainer );

#endif