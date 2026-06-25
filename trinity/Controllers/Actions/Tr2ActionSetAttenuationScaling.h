// Copyright © 2020 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"

BLUE_CLASS( Tr2ActionSetAttenuationScaling ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionSetAttenuationScaling( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Link( ITr2ActionController & controller ) override;
	void Unlink() override;
	void Start( ITr2ActionController & controller ) override;
	void StartWithController( ITr2ActionController * controller );

	float GetScalingFactor() const;

protected:
	ITr2ActionController* m_controller;

	std::string m_emitterName;
	std::string m_controllerVariableName;
	float m_scalingFactor;
};

TYPEDEF_BLUECLASS( Tr2ActionSetAttenuationScaling );
