// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionModifier.h"

BLUE_CLASS( EveDistributionModifierProcessLifetime ) :
	public IEveDistributionModifier
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionModifierProcessLifetime( IRoot* lockobj = nullptr );

	DistributionEntityLifeTimeEvent ProcessDistributionModifier( PlacementDataWithIdentifier & placement, float deltaTime, const EveChildUpdateParams& params );

private:
	float m_lifetimeDuration;
	DistributionEntityLifeTimeEvent m_killEvent;
};

TYPEDEF_BLUECLASS( EveDistributionModifierProcessLifetime );
