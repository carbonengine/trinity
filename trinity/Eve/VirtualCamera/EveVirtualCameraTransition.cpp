// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveVirtualCameraTransition.h"
#include "EveVirtualCamera.h"
#include "EveVirtualCameraBehaviour.h"
#include "Curves/Tr2CurveScalar.h"

EveVirtualCameraTransitionBase::EveVirtualCameraTransitionBase( IRoot* lockobj ) :
	m_sourceCamera(),
	m_targetCamera(),
	m_transitionCamera()
{
}
EveVirtualCameraTransitionBase::~EveVirtualCameraTransitionBase()
{
}

EveVirtualCameraPtr EveVirtualCameraTransitionBase::GetCamera()
{
	if( IsComplete() )
	{
		return m_targetCamera;
	}
	else
	{
		return m_transitionCamera;
	}
}

void EveVirtualCameraTransitionBase::SetSource( const EveVirtualCameraPtr& camera )
{
	m_sourceCamera = camera;
}

void EveVirtualCameraTransitionBase::SetTarget( const EveVirtualCameraPtr& camera )
{
	m_targetCamera = camera;
}

void EveVirtualCameraTransitionBase::Play()
{
	m_transitionCamera.CreateInstance();
	m_transitionCamera->SetName( "transitionCamera" );
	if( m_sourceCamera )
	{
		m_transitionCamera->CopyTransform( m_sourceCamera );
	}
	if( m_targetCamera )
	{
		m_targetCamera->Reset();
	}
	m_transitionCamera->Play();
}

void EveVirtualCameraTransitionBase::Stop()
{
	if( m_targetCamera )
	{
		m_targetCamera->Play();
	}
	if( m_sourceCamera )
	{
		m_sourceCamera->Pause();
	}
	if( m_transitionCamera )
	{
		m_transitionCamera->Pause();
	}
}

void EveVirtualCameraTransitionBase::Update( float deltaTime )
{
	if( m_transitionCamera )
	{
		m_transitionCamera->Update( deltaTime );
	}
	if( IsComplete() )
	{
		Stop();
	}
}

EveVirtualCameraTransitionCut::EveVirtualCameraTransitionCut( IRoot* lockobj ) :
	EveVirtualCameraTransitionBase( lockobj )
{
}

EveVirtualCameraTransitionCut::~EveVirtualCameraTransitionCut()
{
}

bool EveVirtualCameraTransitionCut::IsComplete() const
{
	return true;
}

void EveVirtualCameraTransitionCut::Update( float deltaTime )
{
	EveVirtualCameraTransitionBase::Update( deltaTime );
}

EveVirtualCameraTransitionLerp::EveVirtualCameraTransitionLerp( IRoot* lockobj ) :
	EveVirtualCameraTransitionBase( lockobj ),
	m_transitionTime( 1.0f ),
	m_localTime(),
	m_transitionCurve()
{
	m_transitionCurve.CreateInstance();
	m_transitionCurve->AddKey( 0.0f, 0.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_transitionCurve->AddKey( 1.0f, 1.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
}

EveVirtualCameraTransitionLerp::~EveVirtualCameraTransitionLerp()
{
}

bool EveVirtualCameraTransitionLerp::IsComplete() const
{
	return m_localTime > m_transitionTime;
}

void EveVirtualCameraTransitionLerp::Play()
{
	m_localTime = 0.0;
	EveVirtualCameraTransitionBase::Play();
	if( m_targetCamera )
	{
		m_targetCamera->UpdateToLocalTime( -m_transitionTime );
		m_targetCamera->Play();
	}
}

void EveVirtualCameraTransitionLerp::Update( float deltaTime )
{
	m_localTime += deltaTime;
	if( m_transitionCamera && m_sourceCamera && m_targetCamera )
	{
		auto lerpAmount = 1.0f;
		if( m_transitionTime > 0.0f )
		{
			if( m_transitionCurve )
			{
				lerpAmount = std::max( 0.0f, min( 1.0f, m_transitionCurve->GetValue( m_localTime / m_transitionTime ) ) );
			}
			else
			{
				lerpAmount = std::max( 0.0f, min( 1.0f, m_localTime / m_transitionTime ) );
			}
		}
		m_transitionCamera->UpdateExternal(
			Lerp( m_sourceCamera->GetPosition(), m_targetCamera->GetPosition(), lerpAmount ),
			Lerp( m_sourceCamera->GetPointOfInterest(), m_targetCamera->GetPointOfInterest(), lerpAmount ),
			m_sourceCamera->GetFov() + ( m_targetCamera->GetFov() - m_sourceCamera->GetFov() ) * lerpAmount,
			m_sourceCamera->GetRoll() + ( m_targetCamera->GetRoll() - m_sourceCamera->GetRoll() ) * lerpAmount );
	}
	EveVirtualCameraTransitionBase::Update( deltaTime );
}

float EveVirtualCameraTransitionLerp::GetTransitionTime() const
{
	return m_transitionTime;
}

void EveVirtualCameraTransitionLerp::SetTransitionTime( float value )
{
	m_transitionTime = value;
}
