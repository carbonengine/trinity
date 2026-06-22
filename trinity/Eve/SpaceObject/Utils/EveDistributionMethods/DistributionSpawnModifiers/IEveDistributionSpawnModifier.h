// Copyright © 2025 CCP ehf.

#pragma once
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "../DistributionAttributeModifiers/IEveDistributionModifier.h"

BLUE_DECLARE_STRUCTURE_LIST( PlacementDataWithIdentifier );

BLUE_INTERFACE( IEveDistributionSpawnModifier ) :
	public IRoot
{
public:
	virtual void ProcessSpawnModifier( PlacementDataWithIdentifier & placement, size_t numPlacements ) = 0;
};
