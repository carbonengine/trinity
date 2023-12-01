#pragma once

#include "Controllers/ITr2ControllerOwner.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveVirtualCamera );
BLUE_DECLARE_VECTOR( EveVirtualCamera );
BLUE_DECLARE( EveVirtualCameraTransitionBase );
BLUE_DECLARE( EveUpdateContext );

BLUE_CLASS( EveVirtualCameraSystem ) :
	public IInitialize,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraSystem( IRoot* lockobj = NULL );
	~EveVirtualCameraSystem();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	EveVirtualCameraPtr GetCurrentCamera();
	EveVirtualCameraPtr GetMainCamera();

	// Add camera, unless it's the external camera or already in this systems cameras list. 
	// Returns true if successfully added.
	bool AddCamera( const EveVirtualCameraPtr& camera );
	
	EveVirtualCameraPtr GetCameraByName( const std::string& cameraName ) const;
	
	void CutToCamera( EveVirtualCamera * camera );
	void LerpToCamera( EveVirtualCamera * camera, float lerpTime=1.0f );

	bool IsExternallyControlled();

	void Update( Be::Time simTime );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

private:
	void SetMainCamera( const EveVirtualCameraPtr& cameraIndex );
	void SetMainCamera( const EveVirtualCameraPtr& cameraIndex, const EveVirtualCameraTransitionBasePtr& transition );

	EveVirtualCameraPtr m_mainCamera;
	PEveVirtualCameraVector m_cameras;
	EveVirtualCameraTransitionBasePtr m_transition;
	Be::Time m_lastUpdate;
	EveVirtualCameraPtr m_externalCamera;
};

TYPEDEF_BLUECLASS( EveVirtualCameraSystem );
