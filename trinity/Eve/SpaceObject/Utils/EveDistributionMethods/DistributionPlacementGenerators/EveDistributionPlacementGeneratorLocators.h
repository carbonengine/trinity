// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "IEveDistributionPlacementGenerators.h"
#include "Eve/SpaceObject/Utils/EveLocatorSets.h"

BLUE_DECLARE_STRUCTURE_LIST( Locator );

BLUE_CLASS( EveDistributionPlacementGeneratorLocators ) :
	public IEveDistributionPlacementGenerators,
	public IBlueStructureListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveDistributionPlacementGeneratorLocators( IRoot* lockobj = nullptr );

	void GetInitialPlacements( std::vector<InitialPlacement> & placements, uint32_t& trackingID ) override;
	bool IsRequestingRegeneration() override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionRulesParent& owner ) override;

	// IBlueStructureListNotify
	void OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list ) override;

private:
	PLocatorStructureList m_locators;
	bool m_requestRegeneration;
};

TYPEDEF_BLUECLASS( EveDistributionPlacementGeneratorLocators );
