// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include <ITr2AudEmitter.h>
#include "Controllers/Tr2Controller.h"
#include "Controllers/Tr2ControllerFloatVariable.h"
#include "ITr2SoundEmitterOwner.h"
#include "Tr2ActionSetAttenuationScaling.h"


Tr2ActionSetAttenuationScaling::Tr2ActionSetAttenuationScaling( IRoot* lockobj ) :
	m_scalingFactor( 1.0f ),
	m_controller( nullptr )
{
}

void Tr2ActionSetAttenuationScaling::Link( ITr2ActionController& controller )
{
	m_controller = &controller;
}

void Tr2ActionSetAttenuationScaling::Unlink()
{
	m_controller = nullptr;
}

void Tr2ActionSetAttenuationScaling::Start( ITr2ActionController& controller )
{
	if( ITr2SoundEmitterOwnerPtr emitters = BlueCastPtr( controller.GetOwner() ) )
	{
		if( auto emitter = emitters->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			emitter->SetAttenuationScalingFactor( GetScalingFactor() );
		}
	}
}

void Tr2ActionSetAttenuationScaling::StartWithController( ITr2ActionController* controller )
{
	if( !controller )
	{
		PyErr_SetString( PyExc_TypeError, "StartWithController expects a Tr2Controller as a parameter." );
		return;
	}
	Start( *controller );
}

// Convert a scaling percentage to float for Wwise. If defined, apply
// a value from a controller variable to the final scaling factor.
float Tr2ActionSetAttenuationScaling::GetScalingFactor() const
{
	float controllerVariableValue = 0;

	if( !m_controllerVariableName.empty() && m_controller != nullptr )
	{
		if( auto var = m_controller->GetFloatVariableByName( m_controllerVariableName.c_str() ) )
		{
			controllerVariableValue = *var;
		}
	}

	if( controllerVariableValue != 0 )
	{
		return m_scalingFactor * controllerVariableValue;
	}
	else
	{
		return m_scalingFactor;
	}
}
