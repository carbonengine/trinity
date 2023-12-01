#include "StdAfx.h"
#include "EveVirtualCameraSystem.h"
#include "EveVirtualCamera.h"
#include "EveVirtualCameraTransition.h"

EveVirtualCameraSystem::EveVirtualCameraSystem( IRoot* lockobj ) :
	PARENTLOCK( m_cameras ),
	m_mainCamera(),
	m_transition(),
	m_lastUpdate(),
	m_externalCamera()
{
	m_externalCamera.CreateInstance();
	m_externalCamera->SetName( "externalCamera" );
	m_externalCamera->SetAnimationTimelineLength( 0.0f );
	m_mainCamera = m_externalCamera;
}

EveVirtualCameraSystem::~EveVirtualCameraSystem()
{
}

bool EveVirtualCameraSystem::Initialize()
{
	return true;
}

EveVirtualCameraPtr EveVirtualCameraSystem::GetCurrentCamera()
{
	if( m_transition )
	{
		return m_transition->GetCamera();
	}
	else
	{
		return GetMainCamera();
	}
}

bool EveVirtualCameraSystem::AddCamera( const EveVirtualCameraPtr& camera )
{
	auto found = camera == m_externalCamera;
	if( !found )
	{
		for( auto it = m_cameras.begin(); it != m_cameras.end(); ++it )
		{
			if( ( *it ) == camera )
			{
				found = true;
				break;
			}
		}
	}
	if( !found )
	{
		m_cameras.Append( camera );
	}
	return !found;
}

EveVirtualCameraPtr EveVirtualCameraSystem::GetMainCamera()
{
	return m_mainCamera;
}

EveVirtualCameraPtr EveVirtualCameraSystem::GetCameraByName( const std::string& name ) const
{
	if( name == m_externalCamera->GetName() )
	{
		return m_externalCamera;
	}
	else
	{
		for( auto it = m_cameras.begin(); it != m_cameras.end(); ++it )
		{
			if( (*it)->GetName() == name )
			{
				return *it;
			}
		}
	}
	return nullptr;
}

void EveVirtualCameraSystem::SetMainCamera( const EveVirtualCameraPtr& camera )
{
	m_transition = nullptr;
	m_mainCamera = camera;
	AddCamera( camera );
}

void EveVirtualCameraSystem::SetMainCamera( const EveVirtualCameraPtr& camera, const EveVirtualCameraTransitionBasePtr& transition )
{
	auto current = GetMainCamera();
	SetMainCamera( camera );
	if(transition)
	{
		transition->SetSource( current );
		transition->SetTarget( GetMainCamera() );
		transition->Play();
	}
	m_transition = transition;
}

void EveVirtualCameraSystem::CutToCamera( EveVirtualCamera * camera )
{
	if( camera && camera != GetMainCamera() )
	{
		EveVirtualCameraTransitionCutPtr transition;
		transition.CreateInstance();
		SetMainCamera( camera, (EveVirtualCameraTransitionBasePtr)transition );
	}
}

void EveVirtualCameraSystem::LerpToCamera( EveVirtualCamera * camera, float transitionTime )
{
	if( camera && camera != GetMainCamera() )
	{
		EveVirtualCameraTransitionLerpPtr transition;
		transition.CreateInstance();
		transition->SetTransitionTime( transitionTime );
		SetMainCamera( camera, (EveVirtualCameraTransitionBasePtr)transition );
	}
}

bool EveVirtualCameraSystem::IsExternallyControlled()
{
	return GetCurrentCamera() == m_externalCamera;
}

void EveVirtualCameraSystem::Update( Be::Time simTime )
{
	if( m_lastUpdate == 0.0f )
	{
		m_lastUpdate = simTime;
	}
	float deltaTime = TimeAsFloat(simTime - m_lastUpdate);
	m_lastUpdate = simTime;

	for( auto it = m_cameras.begin(); it != m_cameras.end(); ++it )
	{
		( *it )->Update( deltaTime );
	}
	if( m_externalCamera )
	{
		m_externalCamera->Update( deltaTime );
	}

    if( m_transition )
	{
		m_transition->Update( deltaTime );
		if( m_transition->IsComplete() )
		{
			m_transition = nullptr;
		}
	}
}

void EveVirtualCameraSystem::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	for ( auto it = m_cameras.begin(); it != m_cameras.end(); ++it )
	{
		( *it )->GetDebugOptions( options );
	}
}

void EveVirtualCameraSystem::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	for( auto it = m_cameras.begin(); it != m_cameras.end(); ++it )
	{
		( *it )->RenderDebugInfo( renderer );
	}
}
