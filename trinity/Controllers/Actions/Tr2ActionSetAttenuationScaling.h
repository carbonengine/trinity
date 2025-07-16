////////////////////////////////////////////////////////////
//
//    Created:   May 2020
//    Copyright: CCP 2020
//

#pragma once

#include "ITr2ControllerAction.h"

BLUE_CLASS( Tr2ActionSetAttenuationScaling ) :
    public ITr2ControllerAction
{
public:
	Tr2ActionSetAttenuationScaling( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Link( Tr2Controller& controller ) override;
	void Unlink() override;
	void Start( Tr2Controller& controller ) override;
	void StartWithController( PyObject * controller );

	float GetScalingFactor() const;

protected:
	Tr2Controller* m_controller;

	std::string m_emitterName;
	std::string m_controllerVariableName;
	float m_scalingFactor;
};

TYPEDEF_BLUECLASS( Tr2ActionSetAttenuationScaling );
