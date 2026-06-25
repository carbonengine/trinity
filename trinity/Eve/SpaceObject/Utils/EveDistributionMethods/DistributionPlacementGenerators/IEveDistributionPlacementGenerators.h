// Copyright © 2025 CCP ehf.

#pragma once
#include "../DistributionSpawners/IEveDistributionSpawner.h"

BLUE_INTERFACE( IEveDistributionPlacementGenerators ) :
	public IRoot
{
	virtual void GetInitialPlacements( std::vector<InitialPlacement> & placements, uint32_t& trackingID ) = 0;
	virtual bool IsRequestingRegeneration() = 0;
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) {};
};
