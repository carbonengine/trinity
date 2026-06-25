// Copyright © 2023 CCP ehf.

#pragma once

#include "Eve/EveUpdateContext.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

#include "IEveFxAttribute.h"


BLUE_CLASS( EveSpaceObjectFxAttributes ) :
	public IEveFxAttribute
{
public:
	EXPOSE_TO_BLUE();
	EveSpaceObjectFxAttributes( IRoot* lockobj = nullptr );
	~EveSpaceObjectFxAttributes();

	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;

private:
	// Controls
	BlueSharedString m_name;

	bool m_initialized;
	float m_activationStrength;
	float m_distanceToShip;
	float m_boundingSphereRadius;
	float m_distanceToChildParent;
	float m_killCount;
	float m_activeTurretCount;
	Vector3 m_generatedShapeEllipsoidCenter;
	Vector3 m_generatedShapeEllipsoidRadius;
	Vector3 m_parentWorldTranslation;
	Quaternion m_parentWorldRotation;
};

TYPEDEF_BLUECLASS( EveSpaceObjectFxAttributes );