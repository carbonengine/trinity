////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#pragma once
#ifndef EveChildSocket_H
#define EveChildSocket_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "SocketParameters/IEveSocketParameter.h"


BLUE_DECLARE( EveChildPlug );
BLUE_DECLARE_IVECTOR( IEveSocketParameter );


BLUE_CLASS( EveChildSocket ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public IInitialize,
	public ITr2DebugRenderable,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveChildSocket( IRoot* lockobj = NULL );
	~EveChildSocket();

	const char* GetPlugResPath() const;
	void SetPlugResPath( const char* resPath );

	void Reload();
	bool AddParameterForExternal( Tr2ExternalParameter& externalParam );
	void BindParameters();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );

	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );

	void GetLocalToWorldTransform( Matrix& transform ) const;

	bool IsAlwaysOn() const;

	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;

	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name );
	void StartControllers();
	void SetInheritProperties( const Color* colorSet );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( Tr2DebugRenderer& renderer );

private:
	bool LoadChild();

	EveChildPlugPtr m_plug;

	BlueSharedString m_name;
	std::string m_plugResPath;

	bool m_display;
	
	PIEveSocketParameterVector m_parameters;
};

TYPEDEF_BLUECLASS( EveChildSocket );

#endif