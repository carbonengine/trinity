////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#pragma once
#ifndef EveChildPlug_H
#define EveChildPlug_H


#include "IEveSpaceObjectChild.h"
#include "IEveEffectChildrenOwner.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"
#include "EveChildInheritProperties.h"
#include "ITr2SoundEmitterOwner.h"


BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );


BLUE_CLASS( EveChildPlug ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2CurveSetOwner,
	public IInitialize,
	public IListNotify,	
	public IEveEffectChildrenOwner,
	public ITr2DebugRenderable,
	public IShaderConfigurer,
	public ITr2SoundEmitterOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildPlug( IRoot* lockobj = NULL );
	~EveChildPlug();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveEffectChildrenOwner
	IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const;
	void AddToEffectChildrenList( IEveSpaceObjectChild* child );
	void RemoveFromEffectChildrenList( IEveSpaceObjectChild* child );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	const char* GetName() const;
	void SetName( const char* name );

	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix& transform ) const;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	void PlayAllCurveSets();
	void StopAllCurveSets();
	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	void UpdateCurveSet( const std::string& name, Be::Time time );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;

	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;

	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name );
	void StartControllers();
	void SetInheritProperties( const Color* colorSet );

	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	// external parameters
	void AddExternalParameter( Tr2ExternalParameter* externalParameter );
	const PTr2ExternalParameterVector& GetExternalParameters() const;

	ITr2AudEmitterPtr FindSoundEmitter( const char* name ) override;

protected:
	PIEveSpaceObjectChildVector m_objects;

	BlueSharedString m_name;

	PITr2ControllerVector m_controllers;
	TrackableStdUnorderedMap<std::string, float> m_controllerVariables;
	PTr2ExternalParameterVector m_externalParameters;
	EveChildInheritPropertiesPtr m_inheritProperties;
	Matrix m_worldTransform;

	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildPlug );

#endif
