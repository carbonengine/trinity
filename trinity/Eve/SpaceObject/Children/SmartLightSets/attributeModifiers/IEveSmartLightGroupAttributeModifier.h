// Copyright © 2025 CCP ehf.

#pragma once

#include "Lights/Tr2Light.h"
#include "Eve/SpaceObject/Utils/EveDistributionMethods/IEveDistributionMethod.h"

BLUE_INTERFACE( IEveSmartLightGroupAttributeModifier ) :
	public IRoot
{
public:
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier ) = 0;
	virtual void ProcessAttributeModifier( Vector3 & attribute, const PlacementDataWithIdentifier& placement, const Vector3& entityPosition, const Vector3& entityDirection, float modifierStrength ) = 0;

	virtual void SetControllerVariable( const char* name, float value ) {};
	virtual void SetInheritProperties( const Color* colorSet ) {};
};
