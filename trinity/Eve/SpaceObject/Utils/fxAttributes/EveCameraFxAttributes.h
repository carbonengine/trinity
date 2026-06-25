// Copyright © 2023 CCP ehf.

#pragma once

#include "Eve/EveUpdateContext.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

#include "IEveFxAttribute.h"


BLUE_CLASS( EveCameraFxAttributes ) :
	public IEveFxAttribute
{
public:
	EXPOSE_TO_BLUE();
	EveCameraFxAttributes( IRoot* lockobj = nullptr );
	~EveCameraFxAttributes();

	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;

private:
	// Controls
	BlueSharedString m_name;

	float m_distanceToCamera;
	float m_lookAngleToObject;

	Vector3 m_objectRotation;
	Vector3 m_rotationWithChildTransform;
	Vector3 m_cameraRotation;
};

TYPEDEF_BLUECLASS( EveCameraFxAttributes );