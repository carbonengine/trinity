////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#pragma once
#ifndef EveChildRef_H
#define EveChildRef_H


#include "IEveSpaceObjectChild.h"
#include "IEveEffectChildrenOwner.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"
#include "TransformModifiers/IEveChildTransformModifier.h"
#include "EveChildInheritProperties.h"
#include "ITr2SoundEmitterOwner.h"


BLUE_CLASS( EveChildRef ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2CurveSetOwner,
	public IInitialize,
	public INotify,
	public IEveEffectChildrenOwner,
	public ITr2DebugRenderable,
	public IShaderConfigurer,
	public ITr2SoundEmitterOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildRef( IRoot* lockobj = NULL );
	~EveChildRef();

	const char* GetResPath() const;
	void SetResPath( const char* resPath );

	void Reload( bool bypassAutoLoadBlocker = false);
    void SetAutoLoadBlocker( bool shouldBlockAutoLoad );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveEffectChildrenOwner
	IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const;
	void AddToEffectChildrenList( IEveSpaceObjectChild* child );
	void RemoveFromEffectChildrenList( IEveSpaceObjectChild* child );
    void SetProceduralContainerVariable( const char *name, float value ) override;

	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );

	void GetLocalToWorldTransform( Matrix& transform ) const;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	void UpdateCurveSet( const std::string& name, Be::Time time );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;
    void PlayAllCurveSets() override;
	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;

	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name );
	void StartControllers();
	void SetInheritProperties( const Color* colorSet );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2SoundEmitterOwner 
	ITr2AudEmitterPtr FindSoundEmitter( const char* name ) override;

protected:
	bool LoadChild();

	IEveSpaceObjectChildPtr m_child;
	BlueSharedString m_name;
	std::string m_resPath;

    bool m_loadChildAutomatically;
	bool m_display;
};

TYPEDEF_BLUECLASS( EveChildRef );

#endif
