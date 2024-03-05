////////////////////////////////////////////////////////////
//
//    Created:   May 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Audio/ITr2AudEmitter.h"
#include "Controllers/Tr2Controller.h"
#include "Controllers/Tr2ControllerFloatVariable.h"
#include "ITr2SoundEmitterOwner.h"
#include "Tr2ActionSetAttenuationScaling.h"


Tr2ActionSetAttenuationScaling::Tr2ActionSetAttenuationScaling( IRoot* lockobj ) :
	m_scalingFactor( 1.0f )
{
}

void Tr2ActionSetAttenuationScaling::Link( Tr2Controller& controller )
{
	m_controller = &controller;
}

void Tr2ActionSetAttenuationScaling::Unlink()
{
	m_controller = nullptr;
}

void Tr2ActionSetAttenuationScaling::Start( Tr2Controller& controller )
{
	if( ITr2SoundEmitterOwnerPtr emitters = BlueCastPtr( controller.GetOwner() ) )
	{
		if( auto emitter = emitters->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			emitter->SetAttenuationScalingFactor( GetScalingFactor() );
		}
	}
}

void Tr2ActionSetAttenuationScaling::StartWithController( PyObject* obj )
{
	Tr2Controller* controller = BluePythonCast<Tr2Controller*>( obj );
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

	if ( !m_controllerVariableName.empty() && m_controller != nullptr )
	{
		if ( auto var = m_controller->GetVariableByName( m_controllerVariableName.c_str()) )
		{
			controllerVariableValue = var->GetValue();
		}
	}

	if ( controllerVariableValue != 0 )
	{
		return m_scalingFactor * controllerVariableValue;
	}
	else
	{
		return m_scalingFactor;
	}
}
